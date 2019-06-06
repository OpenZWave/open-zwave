//-----------------------------------------------------------------------------
//
//	SensorMultilevel.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_SENSOR_MULTILEVEL
//
//	Copyright (c) 2010 Mal Lansell <openzwave@lansell.org>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#include "command_classes/CommandClasses.h"
#include "command_classes/SensorMultilevel.h"
#include "command_classes/MultiInstance.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"

#include "value_classes/ValueDecimal.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			enum SensorMultilevelCmd
			{
				SensorMultilevelCmd_SupportedGet = 0x01,
				SensorMultilevelCmd_SupportedReport = 0x02,
				SensorMultilevelCmd_Get = 0x04,
				SensorMultilevelCmd_Report = 0x05
			};

#define MaxSensorTypes (ValueID_Index_SensorMultiLevel::WaterOxidation +1)

			static char const* c_sensorTypeNames[] =
			{ "Undefined", "Temperature", "General", "Luminance", "Power", "Relative Humidity", "Velocity", "Direction", "Atmospheric Pressure", "Barometric Pressure", "Solar Radiation", "Dew Point", "Rain Rate", "Tide Level", "Weight", "Voltage", "Current", "CO2 Level", "Air Flow", "Tank Capacity", "Distance", "Angle Position", "Rotation", "Water Temperature", "Soil Temperature", "Seismic Intensity", "Seismic Magnitude", "Ultraviolet", "Electrical Resistivity", "Electrical Conductivity",
					"Loudness", "Moisture", "Frequency", "Time", "Target Temperature", "Particulate Matter 2.5", "Formaldehyde CH20-level", "Radon Concentration", "Methane (CH4) Density", "Volatile Organic Compound Level", "CO Level", "Soil Humidity", "Soil Reactivity", "Soil Salinity", "Heart Rate", "Blood Pressure", "Muscle Mass", "Fat Mass", "Bone Mass", "Total Body Water", "Basis Metabolic Rate", "Body Mass Index", "Acceleration X-axis", "Acceleration Y-axis", "Acceleration Z-axis",
					"Smoke Density", "Water Flow", "Water Pressure", "RF Signal Strength", "Particulate Matter 10", "Respiratory Rate", "Relative Mdulation Level", "Boiler Water Temperature", "Domestic Hot Water Temperature", "Outside Temperature", "Exhaust Temperature", "Water Chlorine Level", "Water Acidity", "Water Oxidation" };

			static char const* c_tankCapcityUnits[] =
			{ "l", "cbm", "gal", "" };

			static char const* c_distanceUnits[] =
			{ "m", "cm", "ft", "" };

			static char const* c_anglePositionUnits[] =
			{ "%", "deg N", "deg S", "" };

			static char const* c_seismicIntensityUnits[] =
			{ "mercalli", "EU macroseismic", "liedu", "shindo", "" };

			static char const* c_seismicMagnitudeUnits[] =
			{ "local", "moment", "surface wave", "body wave", "" };

			static char const* c_moistureUnits[] =
			{ "%", "content", "k ohms", "water activity", "" };

//-----------------------------------------------------------------------------
// <SensorMultilevel::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
			bool SensorMultilevel::RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue)
			{
				bool res = false;
				if (GetVersion() > 4)
				{
					if (_requestFlags & RequestFlag_Static)
					{
						Msg* msg = new Msg("SensorMultilevelCmd_SupportedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
						msg->SetInstance(this, _instance);
						msg->Append(GetNodeId());
						msg->Append(2);
						msg->Append(GetCommandClassId());
						msg->Append(SensorMultilevelCmd_SupportedGet);
						msg->Append(GetDriver()->GetTransmitOptions());
						GetDriver()->SendMsg(msg, _queue);
						res = true;
					}
				}

				if (_requestFlags & RequestFlag_Dynamic)
				{
					res |= RequestValue(_requestFlags, 0, _instance, _queue);
				}

				return res;
			}

//-----------------------------------------------------------------------------
// <SensorMultilevel::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
			bool SensorMultilevel::RequestValue(uint32 const _requestFlags, uint16 const _dummy,		// = 0 (not used)
					uint8 const _instance, Driver::MsgQueue const _queue)
			{
				bool res = false;
				if (!m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED))
				{
					Log::Write(LogLevel_Info, GetNodeId(), "SensorMultilevelCmd_Get Not Supported on this node");
					return false;
				}
				if (GetVersion() < 5)
				{
					Msg* msg = new Msg("SensorMultilevelCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
					msg->SetInstance(this, _instance);
					msg->Append(GetNodeId());
					msg->Append(2);
					msg->Append(GetCommandClassId());
					msg->Append(SensorMultilevelCmd_Get);
					msg->Append(GetDriver()->GetTransmitOptions());
					GetDriver()->SendMsg(msg, _queue);
					res = true;
				}
				else
				{
					for (uint8 i = 1; i < MaxSensorTypes; i++)
					{
						Internal::VC::Value* value = GetValue(_instance, i);
						if (value != NULL)
						{
							value->Release();
							Msg* msg = new Msg("SensorMultilevelCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId());
							msg->SetInstance(this, _instance);
							msg->Append(GetNodeId());
							msg->Append(3);
							msg->Append(GetCommandClassId());
							msg->Append(SensorMultilevelCmd_Get);
							msg->Append(i);
							msg->Append(GetDriver()->GetTransmitOptions());
							GetDriver()->SendMsg(msg, _queue);
							res = true;
						}
					}
				}
				return res;
			}

//-----------------------------------------------------------------------------
// <SensorMultilevel::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
			bool SensorMultilevel::HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance	// = 1
					)
			{
				if (SensorMultilevelCmd_SupportedReport == (SensorMultilevelCmd) _data[0])
				{
					string msg = "";

					if (Node* node = GetNodeUnsafe())
					{
						for (uint8 i = 1; i <= (_length - 2); i++)
						{
							for (uint8 j = 0; j < 8; j++)
							{
								if (_data[i] & (1 << j))
								{
									if (msg != "")
										msg += ", ";
									uint8 index = ((i - 1) * 8) + j + 1;
									if (index >= MaxSensorTypes) /* max size for c_sensorTypeNames */
									{
										Log::Write(LogLevel_Warning, GetNodeId(), "SensorType Value was greater than range. Dropping");
										continue;
									}
									msg += c_sensorTypeNames[index];
									Internal::VC::ValueDecimal* value = static_cast<Internal::VC::ValueDecimal*>(GetValue(_instance, index));
									if (value == NULL)
									{
										node->CreateValueDecimal(ValueID::ValueGenre_User, GetCommandClassId(), _instance, index, c_sensorTypeNames[index], "", true, false, "0.0", 0);
									}
								}
							}
						}
					}
					Log::Write(LogLevel_Info, GetNodeId(), "Received SensorMultiLevel supported report from node %d: %s", GetNodeId(), msg.c_str());
					return true;
				}
				else if (SensorMultilevelCmd_Report == (SensorMultilevelCmd) _data[0])
				{
					uint8 scale;
					uint8 precision = 0;
					uint8 sensorType = _data[1];
					string valueStr = ExtractValue(&_data[2], &scale, &precision);

					Node* node = GetNodeUnsafe();
					if (node != NULL)
					{
						char const* units = "";
						switch (sensorType)
						{
							case ValueID_Index_SensorMultiLevel::Temperature:
								units = scale ? "F" : "C";
								break;
							case ValueID_Index_SensorMultiLevel::General:
								units = scale ? "" : "%";
								break;
							case ValueID_Index_SensorMultiLevel::Luminance:
								units = scale ? "lux" : "%";
								break;
							case ValueID_Index_SensorMultiLevel::Power:
								units = scale ? "BTU/h" : "W";
								break;
							case ValueID_Index_SensorMultiLevel::RelativeHumidity:
								units = scale ? "" : "%";
								break;
							case ValueID_Index_SensorMultiLevel::Velocity:
								units = scale ? "mph" : "m/s";
								break;
							case ValueID_Index_SensorMultiLevel::Direction:
								units = "";
								break;
							case ValueID_Index_SensorMultiLevel::AtmosphericPressure:
								units = scale ? "inHg" : "kPa";
								break;
							case ValueID_Index_SensorMultiLevel::BarometricPressure:
								units = scale ? "inHg" : "kPa";
								break;
							case ValueID_Index_SensorMultiLevel::SolarRadiation:
								units = "W/m2";
								break;
							case ValueID_Index_SensorMultiLevel::DewPoint:
								units = scale ? "F" : "C";
								break;
							case ValueID_Index_SensorMultiLevel::RainRate:
								units = scale ? "in/h" : "mm/h";
								break;
							case ValueID_Index_SensorMultiLevel::TideLevel:
								units = scale ? "ft" : "m";
								break;
							case ValueID_Index_SensorMultiLevel::Weight:
								units = scale ? "lb" : "kg";
								break;
							case ValueID_Index_SensorMultiLevel::Voltage:
								units = scale ? "mV" : "V";
								break;
							case ValueID_Index_SensorMultiLevel::Current:
								units = scale ? "mA" : "A";
								break;
							case ValueID_Index_SensorMultiLevel::CO2:
								units = "ppm";
								break;
							case ValueID_Index_SensorMultiLevel::AirFlow:
								units = scale ? "cfm" : "m3/h";
								break;
							case ValueID_Index_SensorMultiLevel::TankCapacity:
							{
								if (scale > 2) /* size of c_tankCapcityUnits minus invalid */
								{
									Log::Write(LogLevel_Warning, GetNodeId(), "Scale Value for c_tankCapcityUnits was greater than range. Setting to empty");
									units = c_tankCapcityUnits[3]; /* empty entry */
								}
								else
								{
									units = c_tankCapcityUnits[scale];
								}
							}
								break;
							case ValueID_Index_SensorMultiLevel::Distance:
							{
								if (scale > 2) /* size of c_distanceUnits minus invalid */
								{
									Log::Write(LogLevel_Warning, GetNodeId(), "Scale Value for c_distanceUnits was greater than range. Setting to empty");
									units = c_distanceUnits[3]; /* empty entry */
								}
								else
								{
									units = c_distanceUnits[scale];
								}
							}
								break;
							case ValueID_Index_SensorMultiLevel::AnglePosition:
							{
								if (scale > 2) /* size of c_anglePositionUnits minus invalid */
								{
									Log::Write(LogLevel_Warning, GetNodeId(), "Scale Value for c_anglePositionUnits was greater than range. Setting to empty");
									units = c_anglePositionUnits[3]; /* empty entry */
								}
								else
								{
									units = c_anglePositionUnits[scale];
								}
							}
								break;
							case ValueID_Index_SensorMultiLevel::Rotation:
								units = scale ? "hz" : "rpm";
								break;
							case ValueID_Index_SensorMultiLevel::WaterTemperature:
								units = scale ? "F" : "C";
								break;
							case ValueID_Index_SensorMultiLevel::SoilTemperature:
								units = scale ? "F" : "C";
								break;
							case ValueID_Index_SensorMultiLevel::SeismicIntensity:
							{
								if (scale > 3) /* size of c_seismicIntensityUnits minus invalid */
								{
									Log::Write(LogLevel_Warning, GetNodeId(), "Scale Value for c_seismicIntensityUnits was greater than range. Setting to empty");
									units = c_seismicIntensityUnits[4]; /* empty entry */
								}
								else
								{
									units = c_seismicIntensityUnits[scale];
								}
							}
								break;
							case ValueID_Index_SensorMultiLevel::SeismicMagnitude:
							{
								if (scale > 3) /* size of c_seismicMagnitudeUnits minus invalid */
								{
									Log::Write(LogLevel_Warning, GetNodeId(), "Scale Value for c_seismicMagnitudeUnits was greater than range. Setting to empty");
									units = c_seismicMagnitudeUnits[4]; /* empty entry */
								}
								else
								{
									units = c_seismicMagnitudeUnits[scale];
								}
							}
								break;
							case ValueID_Index_SensorMultiLevel::Ultraviolet:
								units = "";
								break;
							case ValueID_Index_SensorMultiLevel::ElectricalResistivity:
								units = "ohm";
								break;
							case ValueID_Index_SensorMultiLevel::ElectricalConductivity:
								units = "siemens/m";
								break;
							case ValueID_Index_SensorMultiLevel::Loudness:
								units = scale ? "dBA" : "db";
								break;
							case ValueID_Index_SensorMultiLevel::Moisture:
							{
								if (scale > 3) /* size of c_moistureUnits minus invalid */
								{
									Log::Write(LogLevel_Warning, GetNodeId(), "Scale Value for c_moistureUnits was greater than range. Setting to empty");
									units = c_moistureUnits[4]; /* empty entry */
								}
								else
								{
									units = c_moistureUnits[scale];
								}
							}
								break;
							case ValueID_Index_SensorMultiLevel::Frequency:
								units = scale ? "kHz" : "Hz";
								break;
							case ValueID_Index_SensorMultiLevel::Time:
								units = "s";
								break;
							case ValueID_Index_SensorMultiLevel::TargetTemperature:
								units = scale ? "F" : "C";
								break;
							case ValueID_Index_SensorMultiLevel::PM25:
								units = scale ? "ug/m3" : "mol/m3";
								break;
							case ValueID_Index_SensorMultiLevel::CH2O:
								units = "mol/m3";
								break;
							case ValueID_Index_SensorMultiLevel::RadonConcentration:
								units = scale ? "pCi/l" : "bq/m3";
								break;
							case ValueID_Index_SensorMultiLevel::CH4Density:
								units = "mol/m3";
								break;
							case ValueID_Index_SensorMultiLevel::VOC:
								units = scale ? "ppm" : "mol/m3";
								break;
							case ValueID_Index_SensorMultiLevel::CO:
								units = scale ? "ppm" : "mol/m3";
								break;
							case ValueID_Index_SensorMultiLevel::SoilHumidity:
								units = "%";
								break;
							case ValueID_Index_SensorMultiLevel::SoilReactivity:
								units = "pH";
								break;
							case ValueID_Index_SensorMultiLevel::SoilSalinity:
								units = "mol/m3";
								break;
							case ValueID_Index_SensorMultiLevel::HeartRate:
								units = "bpm";
								break;
							case ValueID_Index_SensorMultiLevel::BloodPressure:
								units = scale ? "mmHg (Diastollic)" : "mmHg (Systollic)";
								break;
							case ValueID_Index_SensorMultiLevel::MuscleMass:
								units = "kg";
								break;
							case ValueID_Index_SensorMultiLevel::FatMass:
								units = "kg";
								break;
							case ValueID_Index_SensorMultiLevel::BoneMass:
								units = "kg";
								break;
							case ValueID_Index_SensorMultiLevel::TBW:
								units = "kg";
								break;
							case ValueID_Index_SensorMultiLevel::BMR:
								units = "J";
								break;
							case ValueID_Index_SensorMultiLevel::BMI:
								units = "";
								break;
							case ValueID_Index_SensorMultiLevel::AccelerationX:
								units = "m/s2";
								break;
							case ValueID_Index_SensorMultiLevel::AccelerationY:
								units = "m/s2";
								break;
							case ValueID_Index_SensorMultiLevel::AccelerationZ:
								units = "m/s2";
								break;
							case ValueID_Index_SensorMultiLevel::SmokeDensity:
								units = "%";
								break;
							case ValueID_Index_SensorMultiLevel::WaterFlow:
								units = "l/h";
								break;
							case ValueID_Index_SensorMultiLevel::WaterPressure:
								units = "kPa";
								break;
							case ValueID_Index_SensorMultiLevel::RFSignalStrength:
								units = scale ? "dBm" : "% (RSSI)";
								break;
							case ValueID_Index_SensorMultiLevel::PM10:
								units = scale ? "ug/m3" : "mol/m3";
								break;
							case ValueID_Index_SensorMultiLevel::RespiratoryRate:
								units = "bpm";
								break;
							case ValueID_Index_SensorMultiLevel::RelativeModulationLevel:
								units = "%";
								break;
							case ValueID_Index_SensorMultiLevel::BoilerWaterTemperature:
								units = "C";
								break;
							case ValueID_Index_SensorMultiLevel::DHWTemperature:
								units = "C";
								break;
							case ValueID_Index_SensorMultiLevel::OutsideTemperature:
								units = "C";
								break;
							case ValueID_Index_SensorMultiLevel::ExhaustTemperature:
								units = "C";
								break;
							case ValueID_Index_SensorMultiLevel::WaterChlorineLevel:
								units = "mg/l";
								break;
							case ValueID_Index_SensorMultiLevel::WaterAcidity:
								units = "pH";
								break;
							case ValueID_Index_SensorMultiLevel::WaterOxidation:
								units = "mV";
								break;
							default:
							{
								Log::Write(LogLevel_Warning, GetNodeId(), "sensorType Value was greater than range. Dropping");
								return false;
							}
								break;

						}

						Internal::VC::ValueDecimal* value = static_cast<Internal::VC::ValueDecimal*>(GetValue(_instance, sensorType));
						if (value == NULL)
						{
							node->CreateValueDecimal(ValueID::ValueGenre_User, GetCommandClassId(), _instance, sensorType, c_sensorTypeNames[sensorType], units, true, false, "0.0", 0);
							value = static_cast<Internal::VC::ValueDecimal*>(GetValue(_instance, sensorType));
						}
						else
						{
							value->SetUnits(units);
						}

						Log::Write(LogLevel_Info, GetNodeId(), "Received SensorMultiLevel report from node %d, instance %d, %s: value=%s%s", GetNodeId(), _instance, c_sensorTypeNames[sensorType], valueStr.c_str(), value->GetUnits().c_str());
						if (value->GetPrecision() != precision)
						{
							value->SetPrecision(precision);
						}
						value->OnValueRefreshed(valueStr);
						value->Release();
						return true;
					}
				}
				return false;
			}

//-----------------------------------------------------------------------------
// <SensorMultilevel::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
			void SensorMultilevel::CreateVars(uint8 const _instance)
			{
				// Don't create anything here. We do it in the report.
			}
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
