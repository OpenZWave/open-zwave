#pragma once

#include "ValueID.h"

using namespace OpenZWave;

namespace OpenZWaveUWP
{
	public enum class ZWValueGenre
	{
		Basic = ValueID::ValueGenre_Basic,
		User = ValueID::ValueGenre_User,
		Config = ValueID::ValueGenre_Config,
		System = ValueID::ValueGenre_System
	};

	public enum class ZWValueType
	{
		Bool = ValueID::ValueType_Bool,
		Byte = ValueID::ValueType_Byte,
		Decimal = ValueID::ValueType_Decimal,
		Int = ValueID::ValueType_Int,
		List = ValueID::ValueType_List,
		Schedule = ValueID::ValueType_Schedule,
		Short = ValueID::ValueType_Short,
		String = ValueID::ValueType_String,
		Button = ValueID::ValueType_Button,
		Raw = ValueID::ValueType_Raw
	};

    public ref class ZWValueID sealed
    {
    public:
		ZWValueID
		(
			uint32 homeId,
			uint8 nodeId,
			ZWValueGenre genre,
			uint8 commandClassId,
			uint8 instance,
			uint8 valueIndex,
			ZWValueType type,
			uint8 pollIntensity
		)
		{
			m_valueId = new ValueID(homeId, nodeId, (ValueID::ValueGenre)genre, commandClassId, instance, valueIndex, (ValueID::ValueType)type);
			
		}

		uint32		GetHomeId() { return m_valueId->GetHomeId(); }
		uint8		GetNodeId() { return m_valueId->GetNodeId(); }
		ZWValueGenre	GetGenre() { return (ZWValueGenre)m_valueId->GetGenre(); }
		uint8		GetCommandClassId() { return m_valueId->GetCommandClassId(); }
		uint8		GetInstance() { return m_valueId->GetInstance(); }
		uint8		GetIndex() { return m_valueId->GetIndex(); }
		ZWValueType	GetType() { return (ZWValueType)m_valueId->GetType(); }
		uint64		GetId() { return m_valueId->GetId(); }

	private:
		ValueID* m_valueId;

	internal:
		ZWValueID(ValueID const& valueId)
		{
			m_valueId = new ValueID(valueId);
		}

		ValueID CreateUnmanagedValueID() { return ValueID(*m_valueId); }

		// Comparison Operators
		bool operator ==	(ZWValueID^ _other) { return((*m_valueId) == (*_other->m_valueId)); }
		bool operator !=	(ZWValueID^ _other) { return((*m_valueId) != (*_other->m_valueId)); }
		bool operator <		(ZWValueID^ _other) { return((*m_valueId) < (*_other->m_valueId)); }
		bool operator >		(ZWValueID^ _other) { return((*m_valueId) > (*_other->m_valueId)); }
    };
}
