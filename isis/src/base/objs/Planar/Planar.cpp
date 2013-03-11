/**
 * @file
 * $Revision: 1.4 $
 * $Date: 2008/05/09 18:49:25 $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are public
 *   domain. See individual third-party library and package descriptions for
 *   intellectual property information,user agreements, and related information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or implied,
 *   is made by the USGS as to the accuracy and functioning of such software
 *   and related material nor shall the fact of distribution constitute any such
 *   warranty, and no responsibility is assumed by the USGS in connection
 *   therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html in a browser or see
 *   the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */

#include "Planar.h"

#include <cmath>
#include <cfloat>

#include "Constants.h"
#include "IException.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "RingPlaneProjection.h"

using namespace std;
namespace Isis {
  /**
   * TODO: correct documentation in this file
   *
   * Constructs an Planar object
   *
   * @param label This argument must be a Label containing the proper mapping
   *              information as indicated in the Projection class. Additionally,
   *              the orthographic projection requires the center azimuth to be
   *              defined in the keyword CenterAzimuth.
   *
   * @param allowDefaults If set to false the constructor expects that a keyword
   *                      of CenterAzimuth will be in the label. Otherwise it
   *                      will attempt to compute the center azimuth using the
   *                      middle of the azimuth range specified in the labels.
   *                      Defaults to false.
   *
   * @throws IException
   */
  Planar::Planar(Pvl &label, bool allowDefaults) :
    RingPlaneProjection::RingPlaneProjection(label) {

    // latitude in ring plane is always zero
    m_radius = 0.0;

    try {
      // Try to read the mapping group
      PvlGroup &mapGroup = label.findGroup("Mapping", Pvl::Traverse);

      // Compute and write the default center azimuth if allowed and
      // necessary
      if ((allowDefaults) && (!mapGroup.hasKeyword("CenterAzimuth"))) {
        double az = (m_minimumAzimuth + m_maximumAzimuth) / 2.0;
        mapGroup += PvlKeyword("CenterAzimuth", toString(az));
      }

      // Compute and write the default center radius if allowed and
      // necessary
      if ((allowDefaults) && (!mapGroup.hasKeyword("CenterRadius"))) {
        double radius = (m_minimumRadius + m_maximumRadius) / 2.0;
        mapGroup += PvlKeyword("CenterRadius", toString(radius));
      }

      // Get the center azimuth  & radius
      m_centerAzimuth = mapGroup["CenterAzimuth"];
      m_centerRadius = mapGroup["CenterRadius"];

      // convert to radians, adjust for azimuth direction
      m_centerAzimuth *= DEG2RAD;
      if (m_azimuthDirection == Clockwise) m_centerAzimuth *= -1.0;
    }
    catch(IException &e) {
      string message = "Invalid label group [Mapping]";
      throw IException(e, IException::Io, message, _FILEINFO_);
    }
  }

  //! Destroys the Planar object
  Planar::~Planar() {
  }

  /**
   * Compares two Projection objects to see if they are equal
   *
   * @param proj Projection object to do comparison on
   *
   * @return bool Returns true if the Projection objects are equal, and false if
   *              they are not
   */
  bool Planar::operator== (const Projection &proj) {
    if (!RingPlaneProjection::operator==(proj)) return false;
    // dont do the below it is a recursive plunge
    //  if (Projection::operator!=(proj)) return false;
    Planar *planar = (Planar *) &proj;
    if ((planar->m_centerAzimuth != m_centerAzimuth) ||
        (planar->m_centerRadius != m_centerRadius)) return false;
    return true;
  }

  /**
    * Returns the name of the map projection, "Planar"
    *
    * @return string Name of projection, "Planar"
    */
   QString Planar::Name() const {
     return "Planar";
   }

   /**
    * Returns the center radius, in meters.
    *
    * TODO: Correct this comment for planar projection, assuming right now scale
    * at center of projection.
    * (believe scale is uniform across planar projection)
    *
    * **NOTE** In the case of Planar projections, there is NO radius
    * that is entirely true to scale. The only true scale for this projection is
    * at the single point, (center radius, center azimuth).
    *
    * @return double The center radius.
    */
  double Planar::TrueScaleRadius() const {
    return m_centerRadius;
    // return 60268000.0;
  }


   /**
    * Returns the center azimuth, in degrees.
    *
    * @return double The center azimuth.
    */
  double Planar::CenterAzimuth() const {
    double dir = 1.0;
    if (m_azimuthDirection == Clockwise) dir = -1.0;
    return m_centerAzimuth * RAD2DEG * dir;;
  }


   /**
    * Returns the center radius, in meters.
    *
    * @return double The center radius.
    */
  double Planar::CenterRadius() const {
    return m_centerRadius;
  }


   /**
    * Returns the version of the map projection
    *
    *
    * @return string Version number
    */
   QString Planar::Version() const {
     return "1.0";
   }


  /**
   * This method is used to set the radius/azimuth (assumed to be of the
   * correct AzimuthDirection, a nd AzimuthDomain. The Set
   * forces an attempted calculation of the projection X/Y values. This may or
   * may not be successful and a status is returned as such.
   *
   * @param radius Radius value to project in meters
   *
   * @param az Azimuth value to project in degrees
   *
   * @return bool
   */
  bool Planar::SetGround(const double radius, const double az) {

    // Convert azimuth to radians & adjust
    m_azimuth = az;
    double azRadians = az * DEG2RAD;
    if (m_azimuthDirection == Clockwise) azRadians *= -1.0;

    // Check to make sure radius is valid
    if (radius < 0) {
      m_good = false;
      // cout << "Unable to set radius. The given radius value ["
      //      << IString(radius) << "] is invalid." << endl;
      // throw IException(IException::Unknown,
      //                  "Unable to set radius. The given radius value ["
      //                  + IString(radius) + "] is invalid.",
      //                  _FILEINFO_);
      return m_good;
    }
    m_radius = radius;


    // Compute helper variables
    double deltaAz = (azRadians - m_centerAzimuth);
//  double coslon = cos(deltaLon);

    // Lat/Lon cannot be projected
//    double g =  m_sinph0 * sinphi + m_cosph0 * cosphi * coslon;
//    if ((g <= 0.0) && (fabs(g) > 1.0e-10)) {
//      m_good = false;
//      return m_good;
//    }

    // Compute the coordinates
    double x = radius * cos(deltaAz);
    double y = radius * sin(deltaAz);

    SetComputedXY(x, y);
    m_good = true;
    return m_good;
  }

  /**
   * This method is used to set the projection x/y. The Set forces an attempted
   * calculation of the corresponding radius/azimuth position. This may or
   * may not be successful and a status is returned as such.
   *
   * @param x X coordinate of the projection in units that are the same as the
   *          radii in the label
   *
   * @param y Y coordinate of the projection in units that are the same as the
   *          radii in the label
   *
   * @return bool
   */
  bool Planar::SetCoordinate(const double x, const double y) {

    // Save the coordinate
    SetXY(x, y);

    // compute radius and azimuth in degrees
    m_radius = sqrt(x*x + y*y);

    if (y == 0.0) 
      m_azimuth = m_azimuth;
    else
      m_azimuth = atan2(y,x)  + m_centerAzimuth;

    m_azimuth *= RAD2DEG;

    // if ( m_azimuth < 0.0 )
    //   m_azimuth += 360.0;

    // Cleanup the azimuth
    if (m_azimuthDirection == Clockwise) m_azimuth *= -1.0;

    // These need to be done for circular type projections
    m_azimuth = To360Domain(m_azimuth);

    if (m_azimuthDomain == 180)
      m_azimuth = To180Domain(m_azimuth);

    m_good = true;
   return m_good;
  }

  /**
   * This method is used to determine the x/y range which completely covers the
   * area of interest specified by the radius/longitude range. This range may be
   * obtained from the labels. The purpose of this method is to return the x/y
   * range so it can be used to compute how large a map may need to be. For
   * example, how big a piece of paper is needed or how large of an image needs
   * to be created. The method may fail as indicated by its return value.
   *
   * @param minX Minimum x projection coordinate which covers the radius/
   *             longitude range specified in the labels.
   *
   * @param maxX Maximum x projection coordinate which covers the radius/
   *             longitude range specified in the labels.
   *
   * @param minY Minimum y projection coordinate which covers the radius/
   *             longitude range specified in the labels.
   *
   * @param maxY Maximum y projection coordinate which covers the radius/
   *             longitude range specified in the labels.
   *
   * @return bool
   */
/*
  bool Planar::XYRange(double &minX, double &maxX,
                             double &minY, double &maxY) {
    double lat, lon;

    // Check the corners of the lat/lon range
    XYRangeCheck(m_minimumLatitude, m_minimumLongitude);
    XYRangeCheck(m_maximumLatitude, m_minimumLongitude);
    XYRangeCheck(m_minimumLatitude, m_maximumLongitude);
    XYRangeCheck(m_maximumLatitude, m_maximumLongitude);

//cout << " ************ WALK LATITUDE ******************\n";
//cout << "MIN LAT: " << m_minimumLatitude << " MAX LAT: " << m_maximumLatitude << "\n";
    // Walk top and bottom edges
    for (lat = m_minimumLatitude; lat <= m_maximumLatitude; lat += 0.01) {
//cout << "WALKED A STEP - lat: " << lat << "\n";
      lat = lat;
      lon = m_minimumLongitude;
      XYRangeCheck(lat, lon);

      lat = lat;
      lon = m_maximumLongitude;
      XYRangeCheck(lat, lon);
//cout << "MIN LAT: " << m_minimumLatitude << " MAX LAT: " << m_maximumLatitude << "\n";
    }

//cout << " ************ WALK LONGITUDE ******************\n";
    // Walk left and right edges
    for (lon = m_minimumLongitude; lon <= m_maximumLongitude; lon += 0.01) {
      lat = m_minimumLatitude;
      lon = lon;
      XYRangeCheck(lat, lon);

      lat = m_maximumLatitude;
      lon = lon;
      XYRangeCheck(lat, lon);
    }

    // Walk the limb
    for (double angle = 0.0; angle <= 360.0; angle += 0.01) {
      double x = m_equatorialRadius * cos(angle * PI / 180.0);
      double y = m_equatorialRadius * sin(angle * PI / 180.0);
      if (SetCoordinate(x, y) == 0) {
        if (m_latitude > m_maximumLatitude) {
          continue;
        }
        if (m_longitude > m_maximumLongitude) {
          continue;
        }
        if (m_latitude < m_minimumLatitude) {
          continue;
        }
        if (m_longitude < m_minimumLongitude) {
          continue;
        }

        if (m_minimumX > x) m_minimumX = x;
        if (m_maximumX < x) m_maximumX = x;
        if (m_minimumY > y) m_minimumY = y;
        if (m_maximumY < y) m_maximumY = y;
        XYRangeCheck(m_latitude, m_longitude);
      }
    }

    // Make sure everything is ordered
    if (m_minimumX >= m_maximumX) return false;
    if (m_minimumY >= m_maximumY) return false;

    // Return X/Y min/maxs
    minX = m_minimumX;
    maxX = m_maximumX;
    minY = m_minimumY;
    maxY = m_maximumY;
    return true;
  }
*/


  /**
   * This method is used to determine the x/y range which completely covers the
   * area of interest specified by the radius/longitude range. This range may be
   * obtained from the labels. The purpose of this method is to return the x/y
   * range so it can be used to compute how large a map may need to be. For
   * example, how big a piece of paper is needed or how large of an image needs
   * to be created. The method may fail as indicated by its return value.
   *
   * @param minX Minimum x projection coordinate which covers the radius/
   *             longitude range specified in the labels.
   *
   * @param maxX Maximum x projection coordinate which covers the radius/
   *             longitude range specified in the labels.
   *
   * @param minY Minimum y projection coordinate which covers the radius/
   *             longitude range specified in the labels.
   *
   * @param maxY Maximum y projection coordinate which covers the radius/
   *             longitude range specified in the labels.
   *
   * @return bool
   */
  bool Planar::XYRange(double &minX, double &maxX,
                             double &minY, double &maxY) {
    
    double rad, az;

    // Check the corners of the rad/az range
    XYRangeCheck(m_minimumRadius, m_minimumAzimuth);
    XYRangeCheck(m_maximumRadius, m_minimumAzimuth);
    XYRangeCheck(m_minimumRadius, m_maximumAzimuth);
    XYRangeCheck(m_maximumRadius, m_maximumAzimuth);

//cout << " ************ WALK RADIUS ******************\n";
//cout << "MIN RAD: " << m_minimumRadius << " MAX LAT: " << m_maximumRadius << "\n";
    // Walk top and bottom edges in half pixel increments
    double radiusInc = 2. * (m_maximumRadius - m_minimumRadius) / PixelResolution();

    for (rad = m_minimumRadius; rad <= m_maximumRadius; rad += radiusInc) {
//cout << "WALKED A STEP - rad: " << rad << "\n";
      rad = rad;
      az = m_minimumAzimuth;
      XYRangeCheck(rad, az);

      rad = rad;
      az = m_maximumAzimuth;
      XYRangeCheck(rad, az);
//cout << "MIN RAD: " << m_minimumRadius << " MAX RAD: " << m_maximumRadius << "\n";
    }

//cout << " ************ WALK AZIMUTH ******************\n";
    // Walk left and right edges
    for (az = m_minimumAzimuth; az <= m_maximumAzimuth; az += 0.01) {
      rad = m_minimumRadius;
      az = az;
      XYRangeCheck(rad, az);

      rad = m_maximumRadius;
      az = az;
      XYRangeCheck(rad, az);
    }

    // Walk the limb 
/*
    for (double angle = 0.0; angle <= 360.0; angle += 0.01) {
      double x = m_equatorialRadius * cos(angle * PI / 180.0);
      double y = m_equatorialRadius * sin(angle * PI / 180.0);
      if (SetCoordinate(x, y) == 0) {
        if (m_latitude > m_maximumLatitude) {
          continue;
        }
        if (m_longitude > m_maximumAzimuth) {
          continue;
        }
        if (m_latitude < m_minimumLatitude) {
          continue;
        }
        if (m_longitude < m_minimumAzimuth) {
          continue;
        }

        if (m_minimumX > x) m_minimumX = x;
        if (m_maximumX < x) m_maximumX = x;
        if (m_minimumY > y) m_minimumY = y;
        if (m_maximumY < y) m_maximumY = y;
        XYRangeCheck(m_latitude, m_longitude);
      }
    } */

    // Make sure everything is ordered
    if (m_minimumX >= m_maximumX) return false;
    if (m_minimumY >= m_maximumY) return false;

    // Return X/Y min/maxs
    // m_maximumX = m_maximumRadius*cos(m_maximumAzimuth);
    // m_minimumX = -m_maximumX;
    // m_maximumY = m_maximumRadius*sin(m_maximumAzimuth);
    // m_minimumY = -m_maximumY;

    minX = m_minimumX;
    maxX = m_maximumX;
    minY = m_minimumY;
    maxY = m_maximumY;

    return true;
  }


  /**
   * This function returns the keywords that this projection uses.
   *
   * @return PvlGroup The keywords that this projection uses
   */
  PvlGroup Planar::Mapping() {
    PvlGroup mapping = RingPlaneProjection::Mapping();

    mapping += PvlKeyword("CenterRadius", toString(m_centerRadius));
    double dir = 1.0;
    if (m_azimuthDirection == Clockwise) dir = -1.0;
    double azDegrees = m_centerAzimuth*RAD2DEG*dir;
    mapping += PvlKeyword("CenterAzimuth", toString(azDegrees));

    return mapping;
  }

  /**
   * This function returns the radius keywords that this projection uses
   *
   * @return PvlGroup The radius keywords that this projection uses
   */
  PvlGroup Planar::MappingRadii() {
    PvlGroup mapping = RingPlaneProjection::MappingRadii();

    if (HasGroundRange()) 
      mapping += m_mappingGrp["CenterRadius"];

    return mapping;
  }


  /**
   * This function returns the azimuth keywords that this projection uses
   *
   * @return PvlGroup The azimuth keywords that this projection uses
   */
  PvlGroup Planar::MappingAzimuths() {
    PvlGroup mapping = RingPlaneProjection::MappingAzimuths();

    if (HasGroundRange()) 
      mapping += m_mappingGrp["CenterAzimuth"];

    return mapping;
  }

} // end namespace isis

/** 
 * This is the function that is called in order to instantiate an 
 * Planar object.
 *  
 * @param lab Cube labels with appropriate Mapping information.
 *  
 * @param allowDefaults If the label does not contain the value for 
 *                      CenterAzimuth, this method indicates
 *                      whether the constructor should compute this value.
 * 
 * @return @b Isis::Projection* Pointer to an Planar projection
 *         object.
 */
extern "C" Isis::Projection *PlanarPlugin(Isis::Pvl &lab,
    bool allowDefaults) {
  return new Isis::Planar(lab, allowDefaults);
}

