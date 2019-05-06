//-----------------------------------------------------------------------------
//
//	UserCode.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_USER_CODE
//
//	Copyright (c) 2012 Greg Satz <satz@iranger.com>
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

#include "tinyxml.h"
#include "command_classes/CommandClasses.h"
#include "command_classes/UserCode.h"
#include "command_classes/NodeNaming.h"
#include "Node.h"
#include "Options.h"
#include "platform/Log.h"

#include "value_classes/ValueShort.h"
#include "value_classes/ValueString.h"
#include "value_classes/ValueRaw.h"

using namespace OpenZWave;

enum UserCodeCmd
{
	UserCodeCmd_Set			= 0x01,
	UserCodeCmd_Get			= 0x02,
	UserCodeCmd_Report		= 0x03,
	UserNumberCmd_Get		= 0x04,
	UserNumberCmd_Report	= 0x05
};

enum
{
	UserCodeIndex_Pos_1			= 1,
	UserCodeIndex_Pos_2			= 2,
	UserCodeIndex_Pos_3			= 3,
	UserCodeIndex_Pos_4			= 4,
	UserCodeIndex_Pos_5			= 5,
	UserCodeIndex_Pos_6			= 6,
	UserCodeIndex_Pos_7			= 7,
	UserCodeIndex_Pos_8			= 8,
	UserCodeIndex_Pos_9			= 9,
	UserCodeIndex_Pos_10		= 10,
	UserCodeIndex_Pos_11		= 11,
	UserCodeIndex_Pos_12		= 12,
	UserCodeIndex_Pos_13		= 13,
	UserCodeIndex_Pos_14		= 14,
	UserCodeIndex_Pos_15		= 15,
	UserCodeIndex_Pos_16		= 16,
	UserCodeIndex_Pos_17		= 17,
	UserCodeIndex_Pos_18		= 18,
	UserCodeIndex_Pos_19		= 19,
	UserCodeIndex_Pos_20		= 20,
	UserCodeIndex_Pos_21		= 21,
	UserCodeIndex_Pos_22		= 22,
	UserCodeIndex_Pos_23		= 23,
	UserCodeIndex_Pos_24		= 24,
	UserCodeIndex_Pos_25		= 25,
	UserCodeIndex_Pos_26		= 26,
	UserCodeIndex_Pos_27		= 27,
	UserCodeIndex_Pos_28		= 28,
	UserCodeIndex_Pos_29		= 29,
	UserCodeIndex_Pos_30		= 30,
	UserCodeIndex_Pos_31		= 31,
	UserCodeIndex_Pos_32		= 32,
	UserCodeIndex_Pos_33		= 33,
	UserCodeIndex_Pos_34		= 34,
	UserCodeIndex_Pos_35		= 35,
	UserCodeIndex_Pos_36		= 36,
	UserCodeIndex_Pos_37		= 37,
	UserCodeIndex_Pos_38		= 38,
	UserCodeIndex_Pos_39		= 39,
	UserCodeIndex_Pos_40		= 40,
	UserCodeIndex_Pos_41		= 41,
	UserCodeIndex_Pos_42		= 42,
	UserCodeIndex_Pos_43		= 43,
	UserCodeIndex_Pos_44		= 44,
	UserCodeIndex_Pos_45		= 45,
	UserCodeIndex_Pos_46		= 46,
	UserCodeIndex_Pos_47		= 47,
	UserCodeIndex_Pos_48		= 48,
	UserCodeIndex_Pos_49		= 49,
	UserCodeIndex_Pos_50		= 50,
	UserCodeIndex_Pos_51		= 51,
	UserCodeIndex_Pos_52		= 52,
	UserCodeIndex_Pos_53		= 53,
	UserCodeIndex_Pos_54		= 54,
	UserCodeIndex_Pos_55		= 55,
	UserCodeIndex_Pos_56		= 56,
	UserCodeIndex_Pos_57		= 57,
	UserCodeIndex_Pos_58		= 58,
	UserCodeIndex_Pos_59		= 59,
	UserCodeIndex_Pos_60		= 60,
	UserCodeIndex_Pos_61		= 61,
	UserCodeIndex_Pos_62		= 62,
	UserCodeIndex_Pos_63		= 63,
	UserCodeIndex_Pos_64		= 64,
	UserCodeIndex_Pos_65		= 65,
	UserCodeIndex_Pos_66		= 66,
	UserCodeIndex_Pos_67		= 67,
	UserCodeIndex_Pos_68		= 68,
	UserCodeIndex_Pos_69		= 69,
	UserCodeIndex_Pos_70		= 70,
	UserCodeIndex_Pos_71		= 71,
	UserCodeIndex_Pos_72		= 72,
	UserCodeIndex_Pos_73		= 73,
	UserCodeIndex_Pos_74		= 74,
	UserCodeIndex_Pos_75		= 75,
	UserCodeIndex_Pos_76		= 76,
	UserCodeIndex_Pos_77		= 77,
	UserCodeIndex_Pos_78		= 78,
	UserCodeIndex_Pos_79		= 79,
	UserCodeIndex_Pos_80		= 80,
	UserCodeIndex_Pos_81		= 81,
	UserCodeIndex_Pos_82		= 82,
	UserCodeIndex_Pos_83		= 83,
	UserCodeIndex_Pos_84		= 84,
	UserCodeIndex_Pos_85		= 85,
	UserCodeIndex_Pos_86		= 86,
	UserCodeIndex_Pos_87		= 87,
	UserCodeIndex_Pos_88		= 88,
	UserCodeIndex_Pos_89		= 89,
	UserCodeIndex_Pos_90		= 90,
	UserCodeIndex_Pos_91		= 91,
	UserCodeIndex_Pos_92		= 92,
	UserCodeIndex_Pos_93		= 93,
	UserCodeIndex_Pos_94		= 94,
	UserCodeIndex_Pos_95		= 95,
	UserCodeIndex_Pos_96		= 96,
	UserCodeIndex_Pos_97		= 97,
	UserCodeIndex_Pos_98		= 98,
	UserCodeIndex_Pos_99		= 99,
	UserCodeIndex_Pos_100		= 100,
	UserCodeIndex_Pos_101		= 101,
	UserCodeIndex_Pos_102		= 102,
	UserCodeIndex_Pos_103		= 103,
	UserCodeIndex_Pos_104		= 104,
	UserCodeIndex_Pos_105		= 105,
	UserCodeIndex_Pos_106		= 106,
	UserCodeIndex_Pos_107		= 107,
	UserCodeIndex_Pos_108		= 108,
	UserCodeIndex_Pos_109		= 109,
	UserCodeIndex_Pos_110		= 110,
	UserCodeIndex_Pos_111		= 111,
	UserCodeIndex_Pos_112		= 112,
	UserCodeIndex_Pos_113		= 113,
	UserCodeIndex_Pos_114		= 114,
	UserCodeIndex_Pos_115		= 115,
	UserCodeIndex_Pos_116		= 116,
	UserCodeIndex_Pos_117		= 117,
	UserCodeIndex_Pos_118		= 118,
	UserCodeIndex_Pos_119		= 119,
	UserCodeIndex_Pos_120		= 120,
	UserCodeIndex_Pos_121		= 121,
	UserCodeIndex_Pos_122		= 122,
	UserCodeIndex_Pos_123		= 123,
	UserCodeIndex_Pos_124		= 124,
	UserCodeIndex_Pos_125		= 125,
	UserCodeIndex_Pos_126		= 126,
	UserCodeIndex_Pos_127		= 127,
	UserCodeIndex_Pos_128		= 128,
	UserCodeIndex_Pos_129		= 129,
	UserCodeIndex_Pos_130		= 130,
	UserCodeIndex_Pos_131		= 131,
	UserCodeIndex_Pos_132		= 132,
	UserCodeIndex_Pos_133		= 133,
	UserCodeIndex_Pos_134		= 134,
	UserCodeIndex_Pos_135		= 135,
	UserCodeIndex_Pos_136		= 136,
	UserCodeIndex_Pos_137		= 137,
	UserCodeIndex_Pos_138		= 138,
	UserCodeIndex_Pos_139		= 139,
	UserCodeIndex_Pos_140		= 140,
	UserCodeIndex_Pos_141		= 141,
	UserCodeIndex_Pos_142		= 142,
	UserCodeIndex_Pos_143		= 143,
	UserCodeIndex_Pos_144		= 144,
	UserCodeIndex_Pos_145		= 145,
	UserCodeIndex_Pos_146		= 146,
	UserCodeIndex_Pos_147		= 147,
	UserCodeIndex_Pos_148		= 148,
	UserCodeIndex_Pos_149		= 149,
	UserCodeIndex_Pos_150		= 150,
	UserCodeIndex_Pos_151		= 151,
	UserCodeIndex_Pos_152		= 152,
	UserCodeIndex_Pos_153		= 153,
	UserCodeIndex_Pos_154		= 154,
	UserCodeIndex_Pos_155		= 155,
	UserCodeIndex_Pos_156		= 156,
	UserCodeIndex_Pos_157		= 157,
	UserCodeIndex_Pos_158		= 158,
	UserCodeIndex_Pos_159		= 159,
	UserCodeIndex_Pos_160		= 160,
	UserCodeIndex_Pos_161		= 161,
	UserCodeIndex_Pos_162		= 162,
	UserCodeIndex_Pos_163		= 163,
	UserCodeIndex_Pos_164		= 164,
	UserCodeIndex_Pos_165		= 165,
	UserCodeIndex_Pos_166		= 166,
	UserCodeIndex_Pos_167		= 167,
	UserCodeIndex_Pos_168		= 168,
	UserCodeIndex_Pos_169		= 169,
	UserCodeIndex_Pos_170		= 170,
	UserCodeIndex_Pos_171		= 171,
	UserCodeIndex_Pos_172		= 172,
	UserCodeIndex_Pos_173		= 173,
	UserCodeIndex_Pos_174		= 174,
	UserCodeIndex_Pos_175		= 175,
	UserCodeIndex_Pos_176		= 176,
	UserCodeIndex_Pos_177		= 177,
	UserCodeIndex_Pos_178		= 178,
	UserCodeIndex_Pos_179		= 179,
	UserCodeIndex_Pos_180		= 180,
	UserCodeIndex_Pos_181		= 181,
	UserCodeIndex_Pos_182		= 182,
	UserCodeIndex_Pos_183		= 183,
	UserCodeIndex_Pos_184		= 184,
	UserCodeIndex_Pos_185		= 185,
	UserCodeIndex_Pos_186		= 186,
	UserCodeIndex_Pos_187		= 187,
	UserCodeIndex_Pos_188		= 188,
	UserCodeIndex_Pos_189		= 189,
	UserCodeIndex_Pos_190		= 190,
	UserCodeIndex_Pos_191		= 191,
	UserCodeIndex_Pos_192		= 192,
	UserCodeIndex_Pos_193		= 193,
	UserCodeIndex_Pos_194		= 194,
	UserCodeIndex_Pos_195		= 195,
	UserCodeIndex_Pos_196		= 196,
	UserCodeIndex_Pos_197		= 197,
	UserCodeIndex_Pos_198		= 198,
	UserCodeIndex_Pos_199		= 199,
	UserCodeIndex_Pos_200		= 200,
	UserCodeIndex_Pos_201		= 201,
	UserCodeIndex_Pos_202		= 202,
	UserCodeIndex_Pos_203		= 203,
	UserCodeIndex_Pos_204		= 204,
	UserCodeIndex_Pos_205		= 205,
	UserCodeIndex_Pos_206		= 206,
	UserCodeIndex_Pos_207		= 207,
	UserCodeIndex_Pos_208		= 208,
	UserCodeIndex_Pos_209		= 209,
	UserCodeIndex_Pos_210		= 210,
	UserCodeIndex_Pos_211		= 211,
	UserCodeIndex_Pos_212		= 212,
	UserCodeIndex_Pos_213		= 213,
	UserCodeIndex_Pos_214		= 214,
	UserCodeIndex_Pos_215		= 215,
	UserCodeIndex_Pos_216		= 216,
	UserCodeIndex_Pos_217		= 217,
	UserCodeIndex_Pos_218		= 218,
	UserCodeIndex_Pos_219		= 219,
	UserCodeIndex_Pos_220		= 220,
	UserCodeIndex_Pos_221		= 221,
	UserCodeIndex_Pos_222		= 222,
	UserCodeIndex_Pos_223		= 223,
	UserCodeIndex_Pos_224		= 224,
	UserCodeIndex_Pos_225		= 225,
	UserCodeIndex_Pos_226		= 226,
	UserCodeIndex_Pos_227		= 227,
	UserCodeIndex_Pos_228		= 228,
	UserCodeIndex_Pos_229		= 229,
	UserCodeIndex_Pos_230		= 230,
	UserCodeIndex_Pos_231		= 231,
	UserCodeIndex_Pos_232		= 232,
	UserCodeIndex_Pos_233		= 233,
	UserCodeIndex_Pos_234		= 234,
	UserCodeIndex_Pos_235		= 235,
	UserCodeIndex_Pos_236		= 236,
	UserCodeIndex_Pos_237		= 237,
	UserCodeIndex_Pos_238		= 238,
	UserCodeIndex_Pos_239		= 239,
	UserCodeIndex_Pos_240		= 240,
	UserCodeIndex_Pos_241		= 241,
	UserCodeIndex_Pos_242		= 242,
	UserCodeIndex_Pos_243		= 243,
	UserCodeIndex_Pos_244		= 244,
	UserCodeIndex_Pos_245		= 245,
	UserCodeIndex_Pos_246		= 246,
	UserCodeIndex_Pos_247		= 247,
	UserCodeIndex_Pos_248		= 248,
	UserCodeIndex_Pos_249		= 249,
	UserCodeIndex_Pos_250		= 250,
	UserCodeIndex_Pos_251		= 251,
	UserCodeIndex_Pos_252		= 252,
	UserCodeIndex_Pos_253		= 253,
	UserCodeIndex_Pos_254		= 254,
	UserCodeIndex_Refresh		= 255,
	UserCodeIndex_RemoveCode	= 256,
	UserCodeIndex_Count			= 257,
	UserCodeIndex_RawValue		= 258,
	UserCodeIndex_RawValueIndex	= 259
};


//-----------------------------------------------------------------------------
// <UserCode::UserCode>
// Constructor
//-----------------------------------------------------------------------------
UserCode::UserCode
(
	uint32 const _homeId,
	uint8 const _nodeId
):
	CommandClass( _homeId, _nodeId ),
	m_queryAll( false ),
	m_currentCode( 0 ),
	m_refreshUserCodes(false)
{
	m_com.EnableFlag(COMPAT_FLAG_UC_EXPOSERAWVALUE, false);
	m_dom.EnableFlag(STATE_FLAG_USERCODE_COUNT, 0);
	SetStaticRequest( StaticRequest_Values );
	Options::Get()->GetOptionAsBool("RefreshAllUserCodes", &m_refreshUserCodes );

}

//-----------------------------------------------------------------------------
// <UserCode::RequestState>
// Nothing to do for UserCode
//-----------------------------------------------------------------------------
bool UserCode::RequestState
(
	uint32 const _requestFlags,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	bool requests = false;
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		requests |= RequestValue( _requestFlags, UserCodeIndex_Count, _instance, _queue );
	}

	if( _requestFlags & RequestFlag_Session )
	{
		if( m_dom.GetFlagByte(STATE_FLAG_USERCODE_COUNT) > 0 )
		{
			m_queryAll = true;
			m_currentCode = 1;
			requests |= RequestValue( _requestFlags, m_currentCode, _instance, _queue );
		}
	}

	return requests;
}

//-----------------------------------------------------------------------------
// <UserCode::RequestValue>
// Nothing to do for UserCode
//-----------------------------------------------------------------------------
bool UserCode::RequestValue
(
	uint32 const _requestFlags,
	uint16 const _userCodeIdx,
	uint8 const _instance,
	Driver::MsgQueue const _queue
)
{
	if( _instance != 1 )
	{
		// This command class doesn't work with multiple instances
		return false;
	}
	if ( !m_com.GetFlagBool(COMPAT_FLAG_GETSUPPORTED) )
	{
		Log::Write(  LogLevel_Info, GetNodeId(), "UserNumberCmd_Get Not Supported on this node");
		return false;
	}
	if( _userCodeIdx == UserCodeIndex_Count )
	{
		// Get number of supported user codes.
		Msg* msg = new Msg( "UserNumberCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
		msg->Append( GetNodeId() );
		msg->Append( 2 );
		msg->Append( GetCommandClassId() );
		msg->Append( UserNumberCmd_Get );
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, _queue );
		return true;
	}
	if (_userCodeIdx == 0)
	{
		Log::Write( LogLevel_Warning, GetNodeId(), "UserCodeCmd_Get with Index 0 not Supported");
		return false;
	}
	if (_userCodeIdx > m_dom.GetFlagByte(STATE_FLAG_USERCODE_COUNT))
	{
		Log::Write( LogLevel_Warning, GetNodeId(), "UserCodeCmd_Get with index %d is greater than max UserCodes", _userCodeIdx);
		return false;
	}
	Msg* msg = new Msg( "UserCodeCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->Append( GetNodeId() );
	msg->Append( 3 );
	msg->Append( GetCommandClassId() );
	msg->Append( UserCodeCmd_Get );
	msg->Append( (_userCodeIdx & 0xFF) );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, _queue );
	return true;
}

//-----------------------------------------------------------------------------
// <UserCode::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool UserCode::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance	// = 1
)
{
	if( UserNumberCmd_Report == (UserCodeCmd)_data[0] )
	{
		m_dom.SetFlagByte(STATE_FLAG_USERCODE_COUNT, _data[1]);
		ClearStaticRequest( StaticRequest_Values );
		if( _data[1] == 0 )
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received User Number report from node %d: Not supported", GetNodeId() );
		}
		else
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received User Number report from node %d: Supported Codes %d (%d)", GetNodeId(), _data[1], _data[1] );
		}

		if( ValueShort* value = static_cast<ValueShort*>( GetValue( _instance, UserCodeIndex_Count ) ) )
		{
			value->OnValueRefreshed( _data[1] );
			value->Release();
		}

		if( Node* node = GetNodeUnsafe() )
		{
			string data;

			for( uint16 i = 0; i <= m_dom.GetFlagByte(STATE_FLAG_USERCODE_COUNT); i++ )
			{
				char str[16];
				if (i == 0)
				{
					snprintf( str, sizeof(str), "Enrollment Code");
					node->CreateValueString( ValueID::ValueGenre_User, GetCommandClassId(), _instance, i, str, "", true, false, data, 0 );
				}
				else
				{
					snprintf( str, sizeof(str), "Code %d:", i);
					node->CreateValueString( ValueID::ValueGenre_User, GetCommandClassId(), _instance, i, str, "", false, false, data, 0 );
				}
				m_userCode[i].status = UserCode_Available;
				/* silly compilers */
				for (int j = 0; j < 10; j++)
					m_userCode[i].usercode[i] = 0;
			}
			if (m_com.GetFlagBool(COMPAT_FLAG_UC_EXPOSERAWVALUE)) {
				node->CreateValueRaw( ValueID::ValueGenre_User, GetCommandClassId(), _instance, UserCodeIndex_RawValue, "Raw UserCode", "", false, false, 0, 0, 0);
				node->CreateValueShort( ValueID::ValueGenre_User, GetCommandClassId(), _instance, UserCodeIndex_RawValueIndex, "Raw UserCode Index", "", false, false, 0, 0);
			}
		}
		return true;
	}
	else if( UserCodeCmd_Report == (UserCodeCmd)_data[0] )
	{
		int i = _data[1];
		Log::Write( LogLevel_Info, GetNodeId(), "Received User Code Report from node %d for User Code %d (%s)", GetNodeId(), i, CodeStatus( _data[2] ).c_str() );

		int8 size = _length - 4;
		if( size > 10 )
		{
			Log::Write( LogLevel_Warning, GetNodeId(), "User Code length %d is larger then maximum 10", size );
			size = 10;
		}
		m_userCode[i].status = (UserCodeStatus)_data[2];
		memcpy(&m_userCode[i].usercode, &_data[3], size);
		if( ValueString* value = static_cast<ValueString*>( GetValue( _instance, i ) ) )
		{
			string data;
			/* Max UserCode Length is 10 */
			Log::Write( LogLevel_Info, GetNodeId(), "User Code Packet is %d", size );
			data.assign((const char*)&_data[3], size);	
			value->OnValueRefreshed( data );
			value->Release();
		}
		if (m_com.GetFlagBool(COMPAT_FLAG_UC_EXPOSERAWVALUE)) {
			if( ValueShort* value = static_cast<ValueShort*>( GetValue( _instance, UserCodeIndex_RawValueIndex ) ) ) {
				value->OnValueRefreshed(i);
				value->Release();
			}
			if( ValueRaw* value = static_cast<ValueRaw*>( GetValue( _instance, UserCodeIndex_RawValue ) ) ) {
				value->OnValueRefreshed(&_data[3], (_length - 4));
				value->Release();
			}
		}

		if( m_queryAll && i == m_currentCode )
		{

			if (m_refreshUserCodes || (_data[2] != UserCode_Available)) {
				if( ++i <= m_dom.GetFlagByte(STATE_FLAG_USERCODE_COUNT) )
				{
					m_currentCode = i;
					RequestValue( 0, m_currentCode, _instance, Driver::MsgQueue_Query );
				}
				else
				{
					m_queryAll = false;
					/* we might have reset this as part of the RefreshValues Button Value */
					Options::Get()->GetOptionAsBool("RefreshAllUserCodes", &m_refreshUserCodes );
				}
			} else {
				Log::Write( LogLevel_Info, GetNodeId(), "Not Requesting additional UserCode Slots as RefreshAllUserCodes is false, and slot %d is available", i);
				m_queryAll = false;
			}
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// <UserCode::SetValue>
// Set a User Code value
//-----------------------------------------------------------------------------
bool UserCode::SetValue
(
	Value const& _value
)
{
	if( (ValueID::ValueType_String == _value.GetID().GetType()) && (_value.GetID().GetIndex() < UserCodeIndex_Refresh) )
	{
		ValueString const* value = static_cast<ValueString const*>(&_value);
		string s = value->GetValue();
		if (s.length() < 4) {
			Log::Write( LogLevel_Warning, GetNodeId(), "UserCode is smaller than 4 digits", value->GetID().GetIndex());
			return false;
		}
		if (s.length() > 10) {
			Log::Write( LogLevel_Warning, GetNodeId(), "UserCode is larger than 10 digits", value->GetID().GetIndex());
			return false;
		}
		uint8 len = (uint8_t)(s.length() & 0xFF);
		if (value->GetID().GetIndex() == 0 || value->GetID().GetIndex() > m_dom.GetFlagByte(STATE_FLAG_USERCODE_COUNT)) {
			Log::Write( LogLevel_Warning, GetNodeId(), "Index %d is out of range of UserCodeCount", value->GetID().GetIndex());
			return false;
		}


		Msg* msg = new Msg( "UserCodeCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->SetInstance( this, _value.GetID().GetInstance() );
		msg->Append( GetNodeId() );
		msg->Append( 4 + len );
		msg->Append( GetCommandClassId() );
		msg->Append( UserCodeCmd_Set );
		msg->Append( (uint8_t)(value->GetID().GetIndex() & 0xFF) );
		msg->Append( UserCode_Occupied );
		for( uint8 i = 0; i < len; i++ )
		{
			msg->Append( s[i] );
		}
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );

		return true;
	}
	if ( (ValueID::ValueType_Button == _value.GetID().GetType()) && (_value.GetID().GetIndex() == UserCodeIndex_Refresh) )
	{
		m_refreshUserCodes = true;
		m_currentCode = 1;
		m_queryAll = true;
		RequestValue( 0, m_currentCode, _value.GetID().GetInstance(), Driver::MsgQueue_Query );
		return true;
	}
	if ( (ValueID::ValueType_Short == _value.GetID().GetType()) && (_value.GetID().GetIndex() == UserCodeIndex_RemoveCode) )
	{
		ValueShort const* value = static_cast<ValueShort const*>(&_value);
		uint8_t index = (uint8_t)(value->GetValue() & 0xFF);
		if (index == 0 || index > m_dom.GetFlagByte(STATE_FLAG_USERCODE_COUNT)) {
			Log::Write( LogLevel_Warning, GetNodeId(), "Index %d is out of range of UserCodeCount", index);
			return false;
		}
		Msg* msg = new Msg( "UserCodeCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->SetInstance( this, _value.GetID().GetInstance() );
		msg->Append( GetNodeId() );
		msg->Append( 8 );
		msg->Append( GetCommandClassId() );
		msg->Append( UserCodeCmd_Set );
		msg->Append( index );
		msg->Append( UserCode_Available );
		for( uint8 i = 0; i < 4; i++ )
		{
			msg->Append( 0 );
		}
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );

		RequestValue(0, index, _value.GetID().GetInstance(), Driver::MsgQueue_Send);

#if 0
		/* Reset Our Local Copy here */
		
		if( ValueString* oldvalue = static_cast<ValueString*>( GetValue(  _value.GetID().GetInstance(), index ) ) )
		{
			string data;
			oldvalue->OnValueRefreshed( data );
			oldvalue->Release();
		}
#endif
		return false;
	}
	if ( (ValueID::ValueType_Short == _value.GetID().GetType()) && (_value.GetID().GetIndex() == UserCodeIndex_RawValueIndex) )
	{
		ValueShort const* value = static_cast<ValueShort const*>(&_value);
		uint16 index = value->GetValue();
		if (index == 0 || index > m_dom.GetFlagByte(STATE_FLAG_USERCODE_COUNT)) {
			Log::Write( LogLevel_Warning, GetNodeId(), "Index %d is out of range of UserCodeCount", index);
			return false;
		}
		if( ValueRaw* oldvalue = static_cast<ValueRaw*>( GetValue(  _value.GetID().GetInstance(), UserCodeIndex_RawValue ) ) )
		{
			oldvalue->OnValueRefreshed((const uint8*)&m_userCode[index].usercode, 10);
			oldvalue->Release();
		}
		return false;
	}
	if ( (ValueID::ValueType_Raw == _value.GetID().GetType()) && (_value.GetID().GetIndex() == UserCodeIndex_RawValue) )
	{
		ValueRaw const* value = static_cast<ValueRaw const*>(&_value);
		uint16 index = 0;
		if( ValueShort* valueindex = static_cast<ValueShort*>( GetValue(  _value.GetID().GetInstance(), UserCodeIndex_RawValueIndex ) ) )
		{
			index = valueindex->GetValue();
		}
		if (index == 0 || index > m_dom.GetFlagByte(STATE_FLAG_USERCODE_COUNT)) {
			Log::Write( LogLevel_Warning, GetNodeId(), "Index %d is out of range of UserCodeCount", index);
			return false;
		}
		uint8 *s = value->GetValue();
		uint8 len = value->GetLength();

		Msg* msg = new Msg( "UserCodeCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true );
		msg->SetInstance( this, _value.GetID().GetInstance() );
		msg->Append( GetNodeId() );
		msg->Append( 4 + len );
		msg->Append( GetCommandClassId() );
		msg->Append( UserCodeCmd_Set );
		msg->Append( (uint8_t)(index & 0xFF) );
		msg->Append( UserCode_Occupied );
		for( uint8 i = 0; i < len; i++ )
		{
			msg->Append( s[i] );
		}
		msg->Append( GetDriver()->GetTransmitOptions() );
		GetDriver()->SendMsg( msg, Driver::MsgQueue_Send );
		RequestValue(0, index, _value.GetID().GetInstance(), Driver::MsgQueue_Send);

		return false;
	}
	
	
	return false;
}

//-----------------------------------------------------------------------------
// <UserCode::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void UserCode::CreateVars
(
	uint8 const _instance

)
{
	if( Node* node = GetNodeUnsafe() )
	{
		node->CreateValueShort( ValueID::ValueGenre_System, GetCommandClassId(), _instance, UserCodeIndex_Count, "Code Count", "", true, false, 0, 0 );
		node->CreateValueButton( ValueID::ValueGenre_System, GetCommandClassId(), _instance, UserCodeIndex_Refresh, "Refresh All UserCodes", 0);
		node->CreateValueShort( ValueID::ValueGenre_System, GetCommandClassId(), _instance, UserCodeIndex_RemoveCode, "Remove User Code", "", false, true, 0, 0);
	}
}
