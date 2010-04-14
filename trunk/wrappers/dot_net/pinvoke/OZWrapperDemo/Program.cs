#region Header
//-----------------------------------------------------------------------------
//
//      Program.cs
//
//      Program class for CSharp minimal demo program which illustrates how to 
//      access the OpenZwave dll.  The OpenZwaveDll.dll file must be in the location
//      where this program runs.  This project will copy the dll file to the debug directory
//      after this project is built.  Make sure that the OpenZwaveDll C++ project is built
//      prior to building this project.
//
//      This example simply catches any notification events from the OpenZWave Manager and
//      prints some of the contents of the notification to the log file (see the code).  Press
//      any key to end the program.
//
//      Copyright (c) 2010 Doug Brown <djbrown2001@gmail.com>
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
#endregion Header

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;

using SynapsityAZ.Utils;
using OpenZWaveWrapper;

namespace OZWrapperDemo
{
    class Program
    {
        private static ZWaveManager manager;

        public static void Main(string[] args)
        {

            Log.Create(@"UnmanagedTestLog.txt", Log.Lvl.Debug);
            ZWaveManager.NotificationDelegate NotifyCb = new ZWaveManager.NotificationDelegate(OnNotify);
            manager = new ZWaveManager(@"../../../../config/", "");

            manager.AddWatcher(NotifyCb, IntPtr.Zero);
            manager.AddDriver("COM3");
           
            Console.ReadKey(true);

            manager.Dispose();
        }

        public static void OnNotify(IntPtr pNotification, IntPtr pContext)
        {
            ZWaveNotification notification = new ZWaveNotification(pNotification);
            ZWaveValueId valueId = new ZWaveValueId(notification.ValueId);
            String strLog =
                      "Notification type: " + notification.Type.ToString() +
                      "  Node: " + notification.NodeId.ToString() +
                      "  HomeId: " + notification.HomeId.ToString("X8") +
                      "  ValuIdType: " + valueId.Type.ToString();
            if (notification.Type == ZWaveNotification.NotificationType.Type_DriverReady)
            {
                Log.Write(strLog);
                return;
            }
            IntPtr iptr;
            switch (valueId.Type)
            {
                case ZWaveValueId.ValueType.ValueType_Bool:
                    iptr = manager.GetValueBool(valueId.ValueIdRef);
                    if (iptr != IntPtr.Zero)
                    {
                        ZWaveValueBool zwBool = new ZWaveValueBool(iptr);
                        strLog += ("\n  Value BoolPtr: " + zwBool.GetAsString());
                    }
                    break;
                case ZWaveValueId.ValueType.ValueType_String:
                    iptr = manager.GetValueString(valueId.ValueIdRef);
                    if (iptr != IntPtr.Zero)
                    {
                        ZWaveValueString zwString = new ZWaveValueString(iptr);
                        strLog += ("\n  Value String: " + zwString.GetAsString());
                    }
                    break;
                case ZWaveValueId.ValueType.ValueType_Byte:
                    iptr = manager.GetValueByte(valueId.ValueIdRef);
                    if (iptr != IntPtr.Zero)
                    {
                        ZWaveValueByte zwByte = new ZWaveValueByte(iptr);
                        strLog += ("\n  Value Byte: " + zwByte.GetAsString());
                    }
                    break;
                case ZWaveValueId.ValueType.ValueType_Int:
                    iptr = manager.GetValueInt(valueId.ValueIdRef);
                    if (iptr != IntPtr.Zero)
                    {
                        ZWaveValueInt zwInt = new ZWaveValueInt(iptr);
                        strLog += ("\n  Value Int: " + zwInt.GetAsString());
                    }
                    break;
                case ZWaveValueId.ValueType.ValueType_Short:
                    iptr = manager.GetValueShort(valueId.ValueIdRef);
                    if (iptr != IntPtr.Zero)
                    {
                        ZWaveValueShort zwShort = new ZWaveValueShort(iptr);
                        strLog += ("\n  Value Short: " + zwShort.GetAsString());
                    }
                    break;
                case ZWaveValueId.ValueType.ValueType_Decimal:
                    iptr = manager.GetValueDecimal(valueId.ValueIdRef);
                    if (iptr != IntPtr.Zero)
                    {
                        ZWaveValueDecimal zwDec = new ZWaveValueDecimal(iptr);
                        strLog += ("\n  Value Decimal: " + zwDec.GetAsString());
                    }
                    break;
                case ZWaveValueId.ValueType.ValueType_List:
                    iptr = manager.GetValueList(valueId.ValueIdRef);
                    if (iptr != IntPtr.Zero)
                    {
                        ZWaveValueList zwList = new ZWaveValueList(iptr);
                        strLog += ("\n  Value List: " + zwList.GetAsString());
                    }
                    break;
                default:
                    break;
            } //switch

            Log.Write(strLog);

        }
    }
}
