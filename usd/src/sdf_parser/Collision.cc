/*
 * Copyright (C) 2022 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include "Collision.hh"

#include <string>

#include <ignition/math/Pose3.hh>

// TODO(adlarkin) this is to remove deprecated "warnings" in usd, these warnings
// are reported using #pragma message so normal diagnostic flags cannot remove
// them. This workaround requires this block to be used whenever usd is
// included.
#pragma push_macro ("__DEPRECATED")
#undef __DEPRECATED
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdPhysics/collisionAPI.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#pragma pop_macro ("__DEPRECATED")

#include "sdf/Collision.hh"
#include "sdf/Surface.hh"
#include "../UsdUtils.hh"
#include "Geometry.hh"

namespace sdf
{
// Inline bracket to help doxygen filtering.
inline namespace SDF_VERSION_NAMESPACE {
//
namespace usd
{
  UsdErrors ParseSdfCollision(const sdf::Collision &_collision,
      pxr::UsdStageRefPtr &_stage, const std::string &_path)
  {
    UsdErrors errors;
    const pxr::SdfPath sdfCollisionPath(_path);
    auto usdCollisionXform = pxr::UsdGeomXform::Define(
      _stage, sdfCollisionPath);
    if (!usdCollisionXform)
    {
      errors.push_back(UsdError(sdf::usd::UsdErrorCode::FAILED_USD_DEFINITION,
        "Not able to define a Geom Xform at path [" + _path + "]"));
      return errors;
    }

    // Purpose is a builtin attribute with 4 posible values.
    // - *default* indicates that a prim has “no special purpose” and should
    //   generally be included in all traversals
    // - *render* should generally only be included when performing a
    //   “final quality” render
    // - *proxy* should generally only be included when performing a lightweight
    //    proxy render (such as OpenGL)
    // - *guide* should generally only be included when an interactive
    //    application has been explicitly asked to “show guides”
    // For collisions we need to define this as guide because we don't need
    // to render anything
    auto collisionPrim = _stage->GetPrimAtPath(sdfCollisionPath);
    collisionPrim.CreateAttribute(pxr::TfToken("purpose"),
        pxr::SdfValueTypeNames->Token, false).Set(pxr::TfToken("guide"));

    ignition::math::Pose3d pose;
    auto poseErrors = usd::PoseWrtParent(_collision, pose);
    if (!poseErrors.empty())
    {
      for (const auto &e : poseErrors)
        errors.push_back(e);
      return errors;
    }

    poseErrors = usd::SetPose(pose, _stage, sdfCollisionPath);
    if (!poseErrors.empty())
    {
      for (const auto &e : poseErrors)
        errors.push_back(e);
      errors.push_back(UsdError(UsdErrorCode::SDF_TO_USD_PARSING_ERROR,
            "Unable to set the pose of the prim corresponding to the "
            "SDF collision named [" + _collision.Name() + "]"));
      return errors;
    }

    const auto geometry = *(_collision.Geom());
    const auto geometryPath = std::string(_path + "/geometry");
    auto geomErrors = ParseSdfGeometry(geometry, _stage, geometryPath);
    if (!geomErrors.empty())
    {
      errors.insert(errors.end(), geomErrors.begin(), geomErrors.end());
      errors.push_back(UsdError(
        sdf::usd::UsdErrorCode::SDF_TO_USD_PARSING_ERROR,
        "Error parsing geometry attached to _collision [" +
        _collision.Name() + "]"));
      return errors;
    }

    auto geomPrim = _stage->GetPrimAtPath(pxr::SdfPath(geometryPath));
    if (!geomPrim)
    {
      errors.push_back(UsdError(sdf::usd::UsdErrorCode::INVALID_PRIM_PATH,
        "Internal error: unable to get prim at path ["
        + geometryPath + "], but a geom prim should exist at this path"));
      return errors;
    }

    if (!pxr::UsdPhysicsCollisionAPI::Apply(geomPrim))
    {
      errors.push_back(UsdError(sdf::usd::UsdErrorCode::FAILED_PRIM_API_APPLY,
        "Internal error: unable to apply a collision to the prim at path ["
        + geometryPath + "]"));
      return errors;
    }

    if (auto surface = _collision.Surface())
    {
      if (auto friction = surface->Friction())
      {
        if (auto ode = friction->ODE())
        {
          const auto looksPath = pxr::SdfPath("/Looks");
          auto looksPrim = _stage->GetPrimAtPath(looksPath);
          if (!looksPrim)
          {
            looksPrim = _stage->DefinePrim(looksPath, pxr::TfToken("Scope"));
          }

          // This variable will increase with every new material to avoid
          // collision with the names of the materials
          static int i = 0;

          auto materialPath =
            pxr::SdfPath("/Looks/MaterialPhysics_" + std::to_string(i));
          i++;

          pxr::UsdShadeMaterial materialUsd;
          auto usdMaterialPrim = _stage->GetPrimAtPath(materialPath);
          if (!usdMaterialPrim)
          {
            materialUsd = pxr::UsdShadeMaterial::Define(_stage, materialPath);
            usdMaterialPrim = _stage->GetPrimAtPath(materialPath);
          }
          else
          {
            materialUsd = pxr::UsdShadeMaterial(usdMaterialPrim);
          }

          const pxr::TfToken appliedSchemaNamePhysicsMaterialRootAPI(
            "PhysicsMaterialAPI");
          const pxr::TfToken appliedSchemaNamePhysxMaterialAPI(
            "PhysxMaterialAPI");
          pxr::SdfPrimSpecHandle primSpec = pxr::SdfCreatePrimInLayer(
            _stage->GetEditTarget().GetLayer(), materialPath);
          pxr::SdfTokenListOp listOpMaterial;
          // Use ReplaceOperations to append in place.
          listOpMaterial.ReplaceOperations(
            pxr::SdfListOpTypeExplicit,
            0,
            0,
            {appliedSchemaNamePhysicsMaterialRootAPI,
             appliedSchemaNamePhysxMaterialAPI});
          primSpec->SetInfo(
            pxr::UsdTokens->apiSchemas, pxr::VtValue::Take(listOpMaterial));

          usdMaterialPrim.CreateAttribute(
            pxr::TfToken("physics:density"),
            pxr::SdfValueTypeNames->Float, false).Set(1.0f);

          usdMaterialPrim.CreateAttribute(
            pxr::TfToken("physics:dynamicFriction"),
            pxr::SdfValueTypeNames->Float, false).Set(
              static_cast<float>(ode->Mu()));

          usdMaterialPrim.CreateAttribute(
            pxr::TfToken("physics:restitution"),
            pxr::SdfValueTypeNames->Float, false).Set(1.0f);

          usdMaterialPrim.CreateAttribute(
            pxr::TfToken("physics:staticFriction"),
            pxr::SdfValueTypeNames->Float, false).Set(
              static_cast<float>(ode->Mu2()));

          usdMaterialPrim.CreateAttribute(
            pxr::TfToken("physXMaterial:frictionCombineMode"),
            pxr::SdfValueTypeNames->Token, false).Set(pxr::TfToken("average"));

          usdMaterialPrim.CreateAttribute(
            pxr::TfToken("physXMaterial:restitutionCombineMode"),
            pxr::SdfValueTypeNames->Token, false).Set(pxr::TfToken("average"));

          pxr::UsdShadeMaterialBindingAPI(geomPrim).Bind(materialUsd);
        }
      }
    }

    return errors;
  }
}
}
}
