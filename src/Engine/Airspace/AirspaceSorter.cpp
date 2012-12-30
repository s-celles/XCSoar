#include "AirspaceSorter.hpp"
#include "Airspace/Airspaces.hpp"
#include "AbstractAirspace.hpp"
#include "Geo/GeoVector.hpp"

#include <algorithm>

AirspaceSelectInfo::AirspaceSelectInfo(const AbstractAirspace &_airspace)
  :airspace(&_airspace), vec(GeoVector::Invalid())
{
  const TCHAR *name = airspace->GetName();
  four_chars = ((name[0] & 0xff) << 24) +
               ((name[1] & 0xff) << 16) +
               ((name[2] & 0xff) << 8) +
               ((name[3] & 0xff));
}

void
AirspaceSelectInfo::ResetVector()
{
  vec.SetInvalid();
}

const GeoVector &
AirspaceSelectInfo::GetVector(const GeoPoint &location,
                              const TaskProjection &projection) const
{
  if (!vec.IsValid()) {
    const GeoPoint closest_loc = airspace->ClosestPoint(location, projection);
    vec = GeoVector(location, closest_loc);
  }

  return vec;
}

AirspaceSorter::AirspaceSorter(const Airspaces &airspaces,
                               const GeoPoint &_location)
  :projection(airspaces.GetProjection()), location(_location)
{
  m_airspaces_all.reserve(airspaces.size());

  for (auto it = airspaces.begin(); it != airspaces.end(); ++it) {
    const AbstractAirspace &airspace = *it->GetAirspace();
    AirspaceSelectInfo info(airspace);
    m_airspaces_all.push_back(info);
  }

  SortByName(m_airspaces_all);
}

const AirspaceSelectInfoVector&
AirspaceSorter::GetList() const
{
  return m_airspaces_all;
}

void
AirspaceSorter::FilterByClass(AirspaceSelectInfoVector& vec,
                             const AirspaceClass match_class) const
{
  auto filter = [match_class] (const AirspaceSelectInfo &info) {
    return info.airspace->GetType() != match_class;
  };

  vec.erase(std::remove_if(vec.begin(), vec.end(), filter),vec.end());
}

void
AirspaceSorter::FilterByName(AirspaceSelectInfoVector& vec,
                             const unsigned char match_char) const
{
  auto filter = [match_char] (const AirspaceSelectInfo &info) {
    return (((info.four_chars & 0xff000000) >> 24) != match_char);
  };

  vec.erase(std::remove_if(vec.begin(), vec.end(), filter), vec.end());
}

void
AirspaceSorter::FilterByNamePrefix(AirspaceSelectInfoVector &v,
                                   const TCHAR *prefix) const
{
  auto filter = [prefix] (const AirspaceSelectInfo &info) {
    return !info.airspace->MatchNamePrefix(prefix);
  };

  v.erase(std::remove_if(v.begin(), v.end(), filter), v.end());
}

void
AirspaceSorter::FilterByDirection(AirspaceSelectInfoVector& vec,
                                 const Angle direction) const
{
  auto filter = [&, direction] (const AirspaceSelectInfo &info) {
    Angle bearing = info.GetVector(location, projection).bearing;
    fixed direction_error = (bearing - direction).AsDelta().AbsoluteDegrees();
    return direction_error > fixed(18);
  };

  vec.erase(std::remove_if(vec.begin(), vec.end(), filter), vec.end());
}

void
AirspaceSorter::FilterByDistance(AirspaceSelectInfoVector& vec,
                                const fixed distance) const
{
  auto filter = [&, distance] (const AirspaceSelectInfo &info) {
    return info.GetVector(location, projection).distance > distance;
  };

  vec.erase(std::remove_if(vec.begin(), vec.end(), filter), vec.end());
}

void
AirspaceSorter::SortByDistance(AirspaceSelectInfoVector& vec) const
{
  auto compare = [&] (const AirspaceSelectInfo &elem1,
                      const AirspaceSelectInfo &elem2) {
    return elem1.GetVector(location, projection).distance <
           elem2.GetVector(location, projection).distance;
  };

  std::sort(vec.begin(), vec.end(), compare);
}

void
AirspaceSorter::SortByName(AirspaceSelectInfoVector& vec) const
{
  auto compare = [&] (const AirspaceSelectInfo &elem1,
                      const AirspaceSelectInfo &elem2) {
    return elem1.four_chars < elem2.four_chars;
  };

  std::sort(vec.begin(), vec.end(), compare);
}
