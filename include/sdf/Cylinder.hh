/*
 * Copyright 2018 Open Source Robotics Foundation
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
#ifndef SDF_CYLINDER_HH_
#define SDF_CYLINDER_HH_

#include <sdf/Error.hh>
#include <sdf/Element.hh>
#include <sdf/sdf_config.h>

namespace sdf
{
  // Inline bracke to help doxygen filtering.
  inline namespace SDF_VERSION_NAMESPACE {
  //

  // Forward declare private data class.
  class CylinderPrivate;

  /// \brief Cylinder represents a cylinder shape, and is usually accessed
  /// through a Geometry.
  class SDFORMAT_VISIBLE Cylinder
  {
    /// \brief Constructor
    public: Cylinder();

    /// \brief Destructor
    public: virtual ~Cylinder();

    /// \brief Load the cylinder geometry based on a element pointer.
    /// This is *not* the usual entry point. Typical usage of the SDF DOM is
    /// through the Root object.
    /// \param[in] _sdf The SDF Element pointer
    /// \return Errors, which is a vector of Error objects. Each Error includes
    /// an error code and message. An empty vector indicates no error.
    public: Errors Load(ElementPtr _sdf);

    /// \brief Get the cylinder's radius in meters.
    /// \return The radius of the cylinder in meters.
    public: double Radius() const;

    /// \brief Set the cylinder's radius in meters.
    /// \param[in] _radius The radius of the cylinder in meters.
    public: void SetRadius(const double _radius);

    /// \brief Get the cylinder's length in meters.
    /// \return The length of the cylinder in meters.
    public: double Length() const;

    /// \brief Set the cylinder's length in meters.
    /// \param[in] _length The length of the cylinder in meters.
    public: void SetLength(const double _length);

    /// \brief Get a pointer to the SDF element that was used during
    /// load.
    /// \return SDF element pointer. The value will be nullptr if Load has
    /// not been called.
    public: sdf::ElementPtr Element() const;

    /// \brief Private data pointer.
    private: CylinderPrivate *dataPtr;
  };
  }
}
#endif
