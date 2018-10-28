/*
    Arms for iSENSE
    Copyright (C) 2018 Engaging Computing Group

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CONFIG_H
#define CONFIG_H

namespace Config {


// The domain name of an API
const String APIHost = "192.168.1.7" ;

// The path the the version of the API to be targeted
const String APIPath = "/api/UCAPS" ;

const String APICertificateFingerprint = "94 BA D8 CA D7 EE 77 AC D1 3D F8 FF 09 E9 72 D6 FE 6A 7D 8C" ;

// The path to the particular endpoint to be targeted within the API
// /projects/2156 on the iSENSE API is Plinko!
const String APIEndpoint = "/Parking/AvailableParking" ;

// Port to use in connection to API
const uint16_t APIPort = 3000 ;

/*
    TODO:
    -- add a way to specify the display method (linear, logarithmic, binary)
    -- add more display methods
*/

// FIXME: This may not be the best way to do this
const String targetKey[] = {"data", "0", "AvailableSpaces" } ;

// The expected range of the retrieved value. The servo arm will be adjusted to show how far
// the retrieved values is between these values. If the value is outside this range, the servo
// arm will be moved to either extreme position (0 for <= low bound, 180 for >= high bound)
const double displayLowBound = 0.0;
const double displayHighBound = 223.0 ;

} // namespace Config

#endif // CONFIG_H
