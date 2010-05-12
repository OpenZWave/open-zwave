#include "zwave.h"

//-----------------------------------------------------------------------------
//
//      zwave.cpp
//
//      Cli/C++ wrapper for the C++ OpenZWave Manager class
//
//      Copyright (c) 2010 Amer Harb <harb_amer@hotmail.com>
//
//      SOFTWARE NOTICE AND LICENSE
//
//      This file is part of OpenZWave.
//
//      OpenZWave is free software: you can redistribute it and/or modify
//      it under the terms of the GNU Lesser General Public License as published
//      by the Free Software Foundation, either version 3 of the License,
//      or (at your option) any later version.
//
//      OpenZWave is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU Lesser General Public License for more details.
//
//      You should have received a copy of the GNU Lesser General Public License
//      along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

using namespace OpenZWaveDotNET;
using namespace OpenZWave;
using namespace Runtime::InteropServices;

void ZWManager::Create(String^ configPath,String^userPath){

			const char * configP = (const char*)(Marshal::StringToHGlobalAnsi(configPath)).ToPointer();
			const char * userP = (const char*)(Marshal::StringToHGlobalAnsi(userPath)).ToPointer();
			Manager::Create(configP,userP);
			OnNot = gcnew OnNotificationFromUnmanagedDelegate(this,&ZWManager::OnNotificationFromUnmanaged);
			gch = GCHandle::Alloc(OnNot); 
			IntPtr ip = Marshal::GetFunctionPointerForDelegate(OnNot);
			Manager::Get()->AddWatcher((Manager::pfnOnNotification_t)ip.ToPointer(),NULL);
		}


void  ZWManager::OnNotificationFromUnmanaged(Notification* _notification,void* _context){

	ZWNotification^ notification = gcnew ZWNotification();

	try{
	Monitor::TryEnter( criticalSection );
	try{
		notification->m_type= (uint8)_notification->GetType();
		ValueID v = _notification->GetValueID();
		switch( _notification->GetType() )
		{
			case Notification::Type_ValueAdded:
			{
				notification->SetValueId(gcnew ZWValueID(v.GetHomeId(),v.GetNodeId(),v.GetGenre(),v.GetCommandClassId(),v.GetInstance(),v.GetIndex(),v.GetType()));
				break;
			}
			case Notification::Type_ValueRemoved:
			{
				notification->SetValueId(gcnew ZWValueID(v.GetHomeId(),v.GetNodeId(),v.GetGenre(),v.GetCommandClassId(),v.GetInstance(),v.GetIndex(),v.GetType()));
				break;
			}
			case Notification::Type_ValueChanged:
			{
				notification->SetValueId(gcnew ZWValueID(v.GetHomeId(),v.GetNodeId(),v.GetGenre(),v.GetCommandClassId(),v.GetInstance(),v.GetIndex(),v.GetType()));
				break;
			}
			case Notification::Type_Group:
			{
				notification->SetValueId(gcnew ZWValueID(v.GetHomeId(),v.GetNodeId(),0,0,0,0,0));
				notification->SetGroupIdx(_notification->GetGroupIdx());
				break;
			}
			case Notification::Type_NodeAdded:
			{
				notification->SetValueId(gcnew ZWValueID(v.GetHomeId(), v.GetNodeId(),0,0,0,0,0));
				break;
			}
			case Notification::Type_NodeRemoved:
			{
				notification->SetValueId(gcnew ZWValueID(v.GetHomeId(),v.GetNodeId(),0,0,0,0,0));
				break;
			}
			case Notification::Type_NodeStatus:
			{
				notification->SetValueId(gcnew ZWValueID(v.GetHomeId(),v.GetNodeId(),0,0,0,0,0));
				notification->SetStatus(_notification->GetStatus());
				break;
			}
			case Notification::Type_PollingDisabled:
			{
				notification->SetValueId(gcnew ZWValueID(v.GetHomeId(),v.GetNodeId(),0,0,0,0,0));
				break;
			}
			case Notification::Type_PollingEnabled:
			{
				notification->SetValueId(gcnew ZWValueID(v.GetHomeId(),v.GetNodeId(),0,0,0,0,0));
				break;
			}
			case Notification::Type_DriverReady:
			{
				notification->SetValueId(gcnew ZWValueID(v.GetHomeId(),0,0,0,0,0,0));
				break;
			}
		}
		Monitor::Exit( criticalSection );
	}
	catch(exception& e)
	{
		Monitor::Exit( criticalSection );
	}
	}
	catch(exception& e){}

	OnZWNotification(notification);
}


ZWValue^ ZWManager::GetValue( ZWValueID^ vid ){
	 		
			ZWValueID^ zwvalueId = gcnew ZWValueID();
			zwvalueId->CloneFrom(vid);
			ZWValue^ value = gcnew ZWValue(zwvalueId);
			ValueID unmanagedValueID = zwvalueId->CreateUnmanagedValueID();
			void* v;
			switch( zwvalueId->GetType() )
			{
				case ValueID::ValueType_Bool:
				{
					v= Manager::Get()->GetValueBool(unmanagedValueID);
					break;
				}
				case ValueID::ValueType_Byte:
				{
					v= Manager::Get()->GetValueByte(unmanagedValueID);
					break;
				}
				case ValueID::ValueType_Short:
				{
					v= Manager::Get()->GetValueShort(unmanagedValueID);
					break;
				}
				case ValueID::ValueType_Int:
				{
					v= Manager::Get()->GetValueInt(unmanagedValueID);
					break;
				}
				case ValueID::ValueType_Decimal:
				{
					v= Manager::Get()->GetValueDecimal(unmanagedValueID);
					break;
				}
				case ValueID::ValueType_String:
				{
					v= Manager::Get()->GetValueString(unmanagedValueID);
					break;
				}
				case ValueID::ValueType_List:
				{
					v= Manager::Get()->GetValueList(unmanagedValueID);
					break;
				}
				
				
			}
			try
			{
			value->m_label = gcnew String(static_cast<Value*>(v)->GetLabel().c_str());
			value->m_readOnly = static_cast<Value*>(v)->IsReadOnly();
			value->m_units = gcnew String(static_cast<Value*>(v)->GetUnits().c_str());
			value->m_valueString = gcnew String(static_cast<Value*>(v)->GetAsString().c_str());}
			catch(...){}
			return value;
		}



bool ZWManager::SetValue (ZWValueID^ vid, String^ newvalue)
{
			
			switch(vid->GetType()){
				case ValueID::ValueType_Bool:
					ValueBool* valueb ;
						valueb = Manager::Get()->GetValueBool(vid->CreateUnmanagedValueID());
					return valueb->Set(System::Boolean::Parse(newvalue));
					break;
				case ValueID::ValueType_Byte:
					ValueByte* valueby ;
					valueby = Manager::Get()->GetValueByte(vid->CreateUnmanagedValueID());
					return valueby->Set((uint8)System::Byte::Parse(newvalue));
					break;
				case ValueID::ValueType_Int: //int value
					ValueInt* valueint ;
					valueint= Manager::Get()->GetValueInt(vid->CreateUnmanagedValueID());

					return valueint->Set(System::Int32::Parse(newvalue));
					break;
				case ValueID::ValueType_Decimal: //decimal value
					ValueDecimal* valued;
					valued= Manager::Get()->GetValueDecimal(vid->CreateUnmanagedValueID());

					return valued->Set((const char*)(Marshal::StringToHGlobalAnsi(newvalue)).ToPointer());
					break;
				case ValueID::ValueType_String: //string value
					ValueString* values;
					values= Manager::Get()->GetValueString(vid->CreateUnmanagedValueID());

					return values->Set((const char*)(Marshal::StringToHGlobalAnsi(newvalue)).ToPointer());
					break;
				case ValueID::ValueType_Short: //short value
					ValueShort* valuesh;
					valuesh = Manager::Get()->GetValueShort(vid->CreateUnmanagedValueID());
					return valuesh->Set((uint16)System::UInt16::Parse(newvalue));
					break;
				
				default:
					return false;
			}
		}