
struct ZW_Message_Type
{
    enum _enumerated
    {
        Request = 0x00,
        Response = 0x01,
        Invalid = 0xFFFE
    };
    _enumerated _value;
    ZW_Message_Type() : _value(_enumerated::Invalid) {}
    ZW_Message_Type(_enumerated value) : _value(value) {}
    ZW_Message_Type(const int value)
    {
        if (_is_valid(value))
        {
            _value = static_cast<ZW_Message_Type::_enumerated>(value);
        }
        else
        {
            _value = _enumerated::Invalid;
        }
    }
    operator _enumerated() const { return _value; }
    int operator=(const int _val)
    {
        if (_is_valid(_val))
        {
            _value = static_cast<ZW_Message_Type::_enumerated>(_val);
        }
        else
        {
            _value = _enumerated::Invalid;
        }
        return _value;
    }
    static const bool _is_valid(int _var)
    {
        for (size_t index = 0; index < _count; ++index)
        {
            if (_values()[index] == _var)
            {
                return true;
            }
        }
        return false;
    }
    const char *_to_string() const
    {
        for (size_t index = 0; index < _count; ++index)
        {
            if (_values()[index] == _value)
            {
                return _names()[index];
            }
        }
        return NULL;
    }
    const char *_to_full_string() const
    {
        static std::string val("ZW_Message_Type");
        val = "ZW_Message_Type";
        val.append(".").append(_to_string());
        return val.c_str();
    }
    static const size_t _count = 3;
    static const int *_values()
    {
        static const int values[] = {
            (ignore_assign)Request = 0x00,
            (ignore_assign)Response = 0x01,
            (ignore_assign)Invalid = 0xFFFE,
        };
        return values;
    }
    static const char *const *_names()
    {
        static const char *const raw_names[] = {
            "Request = 0x00",
            "Response = 0x01",
            "Invalid = 0xFFFE",
        };
        static char *processed_names[_count];
        static bool initialized = false;
        if (!initialized)
        {
            for (size_t index = 0; index < _count; ++index)
            {
                size_t length = std::strcspn(raw_names[index], " =\t\n\r");
                processed_names[index] = new char[length + 1];
                strncpy(processed_names[index], raw_names[index], length);
                processed_names[index][length] = '\0';
            }
        }
        return processed_names;
    }
};

struct ZW_Msg_Func
{
    enum _enumerated
    {
        Send_Data = 0x01,
        Invalid = 0xFFFE
    };
    _enumerated _value;
    ZW_Msg_Func() : _value(_enumerated::Invalid) {}
    ZW_Msg_Func(_enumerated value) : _value(value) {}
    ZW_Msg_Func(const int value)
    {
        if (_is_valid(value))
        {
            _value = static_cast<ZW_Msg_Func::_enumerated>(value);
        }
        else
        {
            _value = _enumerated::Invalid;
        }
    }
    operator _enumerated() const { return _value; }
    int operator=(const int _val)
    {
        if (_is_valid(_val))
        {
            _value = static_cast<ZW_Msg_Func::_enumerated>(_val);
        }
        else
        {
            _value = _enumerated::Invalid;
        }
        return _value;
    }
    static const bool _is_valid(int _var)
    {
        for (size_t index = 0; index < _count; ++index)
        {
            if (_values()[index] == _var)
            {
                return true;
            }
        }
        return false;
    }
    const char *_to_string() const
    {
        for (size_t index = 0; index < _count; ++index)
        {
            if (_values()[index] == _value)
            {
                return _names()[index];
            }
        }
        return NULL;
    }
    const char *_to_full_string() const
    {
        static std::string val("ZW_Msg_Func");
        val = "ZW_Msg_Func";
        val.append(".").append(_to_string());
        return val.c_str();
    }
    static const size_t _count = 2;
    static const int *_values()
    {
        static const int values[] = {
            (ignore_assign)Send_Data = 0x01,
            (ignore_assign)Invalid = 0xFFFE,
        };
        return values;
    }
    static const char *const *_names()
    {
        static const char *const raw_names[] = {
            "Send_Data = 0x01",
            "Invalid = 0xFFFE",
        };
        static char *processed_names[_count];
        static bool initialized = false;
        if (!initialized)
        {
            for (size_t index = 0; index < _count; ++index)
            {
                size_t length = std::strcspn(raw_names[index], " =\t\n\r");
                processed_names[index] = new char[length + 1];
                strncpy(processed_names[index], raw_names[index], length);
                processed_names[index][length] = '\0';
            }
        }
        return processed_names;
    }
};

struct ZW_Callback_Func
{
    enum _enumerated
    {
        None = 0x00,
        Command_Handler = 0x01,
        Invalid = 0xFFFE
    };
    _enumerated _value;
    ZW_Callback_Func() : _value(_enumerated::Invalid) {}
    ZW_Callback_Func(_enumerated value) : _value(value) {}
    ZW_Callback_Func(const int value)
    {
        if (_is_valid(value))
        {
            _value = static_cast<ZW_Callback_Func::_enumerated>(value);
        }
        else
        {
            _value = _enumerated::Invalid;
        }
    }
    operator _enumerated() const { return _value; }
    int operator=(const int _val)
    {
        if (_is_valid(_val))
        {
            _value = static_cast<ZW_Callback_Func::_enumerated>(_val);
        }
        else
        {
            _value = _enumerated::Invalid;
        }
        return _value;
    }
    static const bool _is_valid(int _var)
    {
        for (size_t index = 0; index < _count; ++index)
        {
            if (_values()[index] == _var)
            {
                return true;
            }
        }
        return false;
    }
    const char *_to_string() const
    {
        for (size_t index = 0; index < _count; ++index)
        {
            if (_values()[index] == _value)
            {
                return _names()[index];
            }
        }
        return NULL;
    }
    const char *_to_full_string() const
    {
        static std::string val("ZW_Callback_Func");
        val = "ZW_Callback_Func";
        val.append(".").append(_to_string());
        return val.c_str();
    }
    static const size_t _count = 3;
    static const int *_values()
    {
        static const int values[] = {
            (ignore_assign)None = 0x00,
            (ignore_assign)Command_Handler = 0x01,
            (ignore_assign)Invalid = 0xFFFE,
        };
        return values;
    }
    static const char *const *_names()
    {
        static const char *const raw_names[] = {
            "None = 0x00",
            "Command_Handler = 0x01",
            "Invalid = 0xFFFE",
        };
        static char *processed_names[_count];
        static bool initialized = false;
        if (!initialized)
        {
            for (size_t index = 0; index < _count; ++index)
            {
                size_t length = std::strcspn(raw_names[index], " =\t\n\r");
                processed_names[index] = new char[length + 1];
                strncpy(processed_names[index], raw_names[index], length);
                processed_names[index][length] = '\0';
            }
        }
        return processed_names;
    }
};

struct ZW_CommandClasses
{
    enum _enumerated
    {
        Alarm = 0x71,
        Antitheft = 0x5D,
        Antitheft_Unlock = 0x7E,
        Application_Capability = 0x57,
        Application_Status = 0x22,
        Association = 0x85,
        Association_Command_Configuration = 0x9B,
        Association_Group_Info = 0x59,
        Authentication = 0xA1,
        Authentication_Media_Write = 0xA2,
        Barrier_Operator = 0x66,
        Basic = 0x20,
        Basic_Tariff_Info = 0x36,
        Basic_Window_Covering = 0x50,
        Battery = 0x80,
        Central_Scene = 0x5B,
        Climate_Control_Schedule = 0x46,
        Clock = 0x81,
        Configuration = 0x70,
        Controller_Replication = 0x21,
        CRC16_Encap = 0x56,
        DCP_Config = 0x3A,
        DCP_Monitor = 0x3B,
        Device_Reset_Locally = 0x5A,
        Door_Lock = 0x62,
        Door_Lock_Logging = 0x4C,
        Energy_Production = 0x90,
        Entry_Control = 0x6F,
        Firmware_Update_MD = 0x7A,
        Generic_Schedule = 0xA3,
        Geographic_Location = 0x8C,
        Grouping_Name = 0x7B,
        Hail = 0x82,
        HRV_Control = 0x38,
        HRV_Status = 0x37,
        Humidity_Control_Mode = 0x6D,
        Humidity_Control_Operating_State = 0x6E,
        Humidity_Control_Setpoint = 0x64,
        Inclusion_Controller = 0x74,
        Indicator = 0x87,
        IP_Association = 0x5C,
        IP_Configuration = 0x9A,
        IR_Repeater = 0xA0,
        Irrigation = 0x6B,
        Language = 0x89,
        Lock = 0x76,
        Mailbox = 0x69,
        Manufacturer_Proprietary = 0x91,
        Manufacturer_Specific = 0x72,
        Mark = 0xEF,
        Meter = 0x32,
        Meter_Pulse = 0x35,
        Meter_Tbl_Config = 0x3C,
        Meter_Tbl_Monitor = 0x3D,
        Meter_Tbl_Push = 0x3E,
        MTP_Window_Covering = 0x51,
        MultiChannel = 0x60,
        MultiChannel_Association = 0x8E,
        MultiCommand = 0x8F,
        Network_Management_Basic = 0x4D,
        Network_Management_Inclusion = 0x34,
        Network_Management_Installation_Maintenance = 0x67,
        Network_Management_Primary = 0x54,
        Network_Management_Proxy = 0x52,
        No_Operation = 0x00,
        Node_Naming = 0x77,
        Node_Provisioning = 0x78,
        Notification = 0x71,
        PowerLevel = 0x73,
        Prepayment = 0x3F,
        Prepayment_Encapsulation = 0x41,
        Proprietary = 0x88,
        Protection = 0x75,
        Rate_Tbl_Config = 0x48,
        Rate_Tbl_Monitor = 0x49,
        Remote_Association = 0x7D,
        Remote_Association_Activate = 0x7C,
        Scene_Activation = 0x2B,
        Scene_Actuator_Conf = 0x2C,
        Scene_Controller_Conf = 0x2D,
        Schedule = 0x53,
        Schedule_Entry_Lock = 0x4E,
        Screen_Attributes = 0x93,
        Screen_MD = 0x92,
        Security_S0 = 0x98,
        Security_S2 = 0x9F,
        Sensor_Alarm = 0x9C,
        Sensor_Binary = 0x30,
        Sensor_Configuration = 0x9E,
        Sensor_Multilevel = 0x31,
        Silence_Alarm = 0x9D,
        Simple_AV_Control = 0x94,
        Sound_Switch = 0x79,
        Supervision = 0x6C,
        Switch_All = 0x27,
        Switch_Binary = 0x25,
        Switch_Color = 0x33,
        Switch_Multilevel = 0x26,
        Switch_Toggle_Binary = 0x28,
        Switch_Toggle_Multilevel = 0x29,
        Tariff_Config = 0x4A,
        Tariff_Tbl_Monitor = 0x4B,
        Thermostat_Fan_Mode = 0x44,
        Thermostat_Fan_State = 0x45,
        Thermostat_Mode = 0x40,
        Thermostat_Operating_State = 0x42,
        Thermostat_Setback = 0x47,
        Thermostat_Setpoint = 0x43,
        Time = 0x8A,
        Time_Parameters = 0x8B,
        Transport_Service = 0x55,
        User_Code = 0x63,
        Version = 0x86,
        Wake_Up = 0x84,
        Window_Covering = 0x6A,
        ZIP = 0x23,
        ZIP_6LoWPAN = 0x4F,
        ZIP_Gateway = 0x5F,
        ZIP_Naming = 0x68,
        ZIP_ND = 0x58,
        ZIP_Portal = 0x61,
        ZWavePlus_Info = 0x5E,
        Invalid = 0xFFFE
    };
    _enumerated _value;
    ZW_CommandClasses() : _value(_enumerated::Invalid) {}
    ZW_CommandClasses(_enumerated value) : _value(value) {}
    ZW_CommandClasses(const int value)
    {
        if (_is_valid(value))
        {
            _value = static_cast<ZW_CommandClasses::_enumerated>(value);
        }
        else
        {
            _value = _enumerated::Invalid;
        }
    }
    operator _enumerated() const { return _value; }
    int operator=(const int _val)
    {
        if (_is_valid(_val))
        {
            _value = static_cast<ZW_CommandClasses::_enumerated>(_val);
        }
        else
        {
            _value = _enumerated::Invalid;
        }
        return _value;
    }
    static const bool _is_valid(int _var)
    {
        for (size_t index = 0; index < _count; ++index)
        {
            if (_values()[index] == _var)
            {
                return true;
            }
        }
        return false;
    }
    const char *_to_string() const
    {
        for (size_t index = 0; index < _count; ++index)
        {
            if (_values()[index] == _value)
            {
                return _names()[index];
            }
        }
        return NULL;
    }
    const char *_to_full_string() const
    {
        static std::string val("ZW_CommandClasses");
        val = "ZW_CommandClasses";
        val.append(".").append(_to_string());
        return val.c_str();
    }
    static const size_t _count = 123;
    static const int *_values()
    {
        static const int values[] = {
            (ignore_assign)Alarm = 0x71,
            (ignore_assign)Antitheft = 0x5D,
            (ignore_assign)Antitheft_Unlock = 0x7E,
            (ignore_assign)Application_Capability = 0x57,
            (ignore_assign)Application_Status = 0x22,
            (ignore_assign)Association = 0x85,
            (ignore_assign)Association_Command_Configuration = 0x9B,
            (ignore_assign)Association_Group_Info = 0x59,
            (ignore_assign)Authentication = 0xA1,
            (ignore_assign)Authentication_Media_Write = 0xA2,
            (ignore_assign)Barrier_Operator = 0x66,
            (ignore_assign)Basic = 0x20,
            (ignore_assign)Basic_Tariff_Info = 0x36,
            (ignore_assign)Basic_Window_Covering = 0x50,
            (ignore_assign)Battery = 0x80,
            (ignore_assign)Central_Scene = 0x5B,
            (ignore_assign)Climate_Control_Schedule = 0x46,
            (ignore_assign)Clock = 0x81,
            (ignore_assign)Configuration = 0x70,
            (ignore_assign)Controller_Replication = 0x21,
            (ignore_assign)CRC16_Encap = 0x56,
            (ignore_assign)DCP_Config = 0x3A,
            (ignore_assign)DCP_Monitor = 0x3B,
            (ignore_assign)Device_Reset_Locally = 0x5A,
            (ignore_assign)Door_Lock = 0x62,
            (ignore_assign)Door_Lock_Logging = 0x4C,
            (ignore_assign)Energy_Production = 0x90,
            (ignore_assign)Entry_Control = 0x6F,
            (ignore_assign)Firmware_Update_MD = 0x7A,
            (ignore_assign)Generic_Schedule = 0xA3,
            (ignore_assign)Geographic_Location = 0x8C,
            (ignore_assign)Grouping_Name = 0x7B,
            (ignore_assign)Hail = 0x82,
            (ignore_assign)HRV_Control = 0x38,
            (ignore_assign)HRV_Status = 0x37,
            (ignore_assign)Humidity_Control_Mode = 0x6D,
            (ignore_assign)Humidity_Control_Operating_State = 0x6E,
            (ignore_assign)Humidity_Control_Setpoint = 0x64,
            (ignore_assign)Inclusion_Controller = 0x74,
            (ignore_assign)Indicator = 0x87,
            (ignore_assign)IP_Association = 0x5C,
            (ignore_assign)IP_Configuration = 0x9A,
            (ignore_assign)IR_Repeater = 0xA0,
            (ignore_assign)Irrigation = 0x6B,
            (ignore_assign)Language = 0x89,
            (ignore_assign)Lock = 0x76,
            (ignore_assign)Mailbox = 0x69,
            (ignore_assign)Manufacturer_Proprietary = 0x91,
            (ignore_assign)Manufacturer_Specific = 0x72,
            (ignore_assign)Mark = 0xEF,
            (ignore_assign)Meter = 0x32,
            (ignore_assign)Meter_Pulse = 0x35,
            (ignore_assign)Meter_Tbl_Config = 0x3C,
            (ignore_assign)Meter_Tbl_Monitor = 0x3D,
            (ignore_assign)Meter_Tbl_Push = 0x3E,
            (ignore_assign)MTP_Window_Covering = 0x51,
            (ignore_assign)MultiChannel = 0x60,
            (ignore_assign)MultiChannel_Association = 0x8E,
            (ignore_assign)MultiCommand = 0x8F,
            (ignore_assign)Network_Management_Basic = 0x4D,
            (ignore_assign)Network_Management_Inclusion = 0x34,
            (ignore_assign)Network_Management_Installation_Maintenance = 0x67,
            (ignore_assign)Network_Management_Primary = 0x54,
            (ignore_assign)Network_Management_Proxy = 0x52,
            (ignore_assign)No_Operation = 0x00,
            (ignore_assign)Node_Naming = 0x77,
            (ignore_assign)Node_Provisioning = 0x78,
            (ignore_assign)Notification = 0x71,
            (ignore_assign)PowerLevel = 0x73,
            (ignore_assign)Prepayment = 0x3F,
            (ignore_assign)Prepayment_Encapsulation = 0x41,
            (ignore_assign)Proprietary = 0x88,
            (ignore_assign)Protection = 0x75,
            (ignore_assign)Rate_Tbl_Config = 0x48,
            (ignore_assign)Rate_Tbl_Monitor = 0x49,
            (ignore_assign)Remote_Association = 0x7D,
            (ignore_assign)Remote_Association_Activate = 0x7C,
            (ignore_assign)Scene_Activation = 0x2B,
            (ignore_assign)Scene_Actuator_Conf = 0x2C,
            (ignore_assign)Scene_Controller_Conf = 0x2D,
            (ignore_assign)Schedule = 0x53,
            (ignore_assign)Schedule_Entry_Lock = 0x4E,
            (ignore_assign)Screen_Attributes = 0x93,
            (ignore_assign)Screen_MD = 0x92,
            (ignore_assign)Security_S0 = 0x98,
            (ignore_assign)Security_S2 = 0x9F,
            (ignore_assign)Sensor_Alarm = 0x9C,
            (ignore_assign)Sensor_Binary = 0x30,
            (ignore_assign)Sensor_Configuration = 0x9E,
            (ignore_assign)Sensor_Multilevel = 0x31,
            (ignore_assign)Silence_Alarm = 0x9D,
            (ignore_assign)Simple_AV_Control = 0x94,
            (ignore_assign)Sound_Switch = 0x79,
            (ignore_assign)Supervision = 0x6C,
            (ignore_assign)Switch_All = 0x27,
            (ignore_assign)Switch_Binary = 0x25,
            (ignore_assign)Switch_Color = 0x33,
            (ignore_assign)Switch_Multilevel = 0x26,
            (ignore_assign)Switch_Toggle_Binary = 0x28,
            (ignore_assign)Switch_Toggle_Multilevel = 0x29,
            (ignore_assign)Tariff_Config = 0x4A,
            (ignore_assign)Tariff_Tbl_Monitor = 0x4B,
            (ignore_assign)Thermostat_Fan_Mode = 0x44,
            (ignore_assign)Thermostat_Fan_State = 0x45,
            (ignore_assign)Thermostat_Mode = 0x40,
            (ignore_assign)Thermostat_Operating_State = 0x42,
            (ignore_assign)Thermostat_Setback = 0x47,
            (ignore_assign)Thermostat_Setpoint = 0x43,
            (ignore_assign)Time = 0x8A,
            (ignore_assign)Time_Parameters = 0x8B,
            (ignore_assign)Transport_Service = 0x55,
            (ignore_assign)User_Code = 0x63,
            (ignore_assign)Version = 0x86,
            (ignore_assign)Wake_Up = 0x84,
            (ignore_assign)Window_Covering = 0x6A,
            (ignore_assign)ZIP = 0x23,
            (ignore_assign)ZIP_6LoWPAN = 0x4F,
            (ignore_assign)ZIP_Gateway = 0x5F,
            (ignore_assign)ZIP_Naming = 0x68,
            (ignore_assign)ZIP_ND = 0x58,
            (ignore_assign)ZIP_Portal = 0x61,
            (ignore_assign)ZWavePlus_Info = 0x5E,
            (ignore_assign)Invalid = 0xFFFE,
        };
        return values;
    }
    static const char *const *_names()
    {
        static const char *const raw_names[] = {
            "Alarm = 0x71",
            "Antitheft = 0x5D",
            "Antitheft_Unlock = 0x7E",
            "Application_Capability = 0x57",
            "Application_Status = 0x22",
            "Association = 0x85",
            "Association_Command_Configuration = 0x9B",
            "Association_Group_Info = 0x59",
            "Authentication = 0xA1",
            "Authentication_Media_Write = 0xA2",
            "Barrier_Operator = 0x66",
            "Basic = 0x20",
            "Basic_Tariff_Info = 0x36",
            "Basic_Window_Covering = 0x50",
            "Battery = 0x80",
            "Central_Scene = 0x5B",
            "Climate_Control_Schedule = 0x46",
            "Clock = 0x81",
            "Configuration = 0x70",
            "Controller_Replication = 0x21",
            "CRC16_Encap = 0x56",
            "DCP_Config = 0x3A",
            "DCP_Monitor = 0x3B",
            "Device_Reset_Locally = 0x5A",
            "Door_Lock = 0x62",
            "Door_Lock_Logging = 0x4C",
            "Energy_Production = 0x90",
            "Entry_Control = 0x6F",
            "Firmware_Update_MD = 0x7A",
            "Generic_Schedule = 0xA3",
            "Geographic_Location = 0x8C",
            "Grouping_Name = 0x7B",
            "Hail = 0x82",
            "HRV_Control = 0x38",
            "HRV_Status = 0x37",
            "Humidity_Control_Mode = 0x6D",
            "Humidity_Control_Operating_State = 0x6E",
            "Humidity_Control_Setpoint = 0x64",
            "Inclusion_Controller = 0x74",
            "Indicator = 0x87",
            "IP_Association = 0x5C",
            "IP_Configuration = 0x9A",
            "IR_Repeater = 0xA0",
            "Irrigation = 0x6B",
            "Language = 0x89",
            "Lock = 0x76",
            "Mailbox = 0x69",
            "Manufacturer_Proprietary = 0x91",
            "Manufacturer_Specific = 0x72",
            "Mark = 0xEF",
            "Meter = 0x32",
            "Meter_Pulse = 0x35",
            "Meter_Tbl_Config = 0x3C",
            "Meter_Tbl_Monitor = 0x3D",
            "Meter_Tbl_Push = 0x3E",
            "MTP_Window_Covering = 0x51",
            "MultiChannel = 0x60",
            "MultiChannel_Association = 0x8E",
            "MultiCommand = 0x8F",
            "Network_Management_Basic = 0x4D",
            "Network_Management_Inclusion = 0x34",
            "Network_Management_Installation_Maintenance = 0x67",
            "Network_Management_Primary = 0x54",
            "Network_Management_Proxy = 0x52",
            "No_Operation = 0x00",
            "Node_Naming = 0x77",
            "Node_Provisioning = 0x78",
            "Notification = 0x71",
            "PowerLevel = 0x73",
            "Prepayment = 0x3F",
            "Prepayment_Encapsulation = 0x41",
            "Proprietary = 0x88",
            "Protection = 0x75",
            "Rate_Tbl_Config = 0x48",
            "Rate_Tbl_Monitor = 0x49",
            "Remote_Association = 0x7D",
            "Remote_Association_Activate = 0x7C",
            "Scene_Activation = 0x2B",
            "Scene_Actuator_Conf = 0x2C",
            "Scene_Controller_Conf = 0x2D",
            "Schedule = 0x53",
            "Schedule_Entry_Lock = 0x4E",
            "Screen_Attributes = 0x93",
            "Screen_MD = 0x92",
            "Security_S0 = 0x98",
            "Security_S2 = 0x9F",
            "Sensor_Alarm = 0x9C",
            "Sensor_Binary = 0x30",
            "Sensor_Configuration = 0x9E",
            "Sensor_Multilevel = 0x31",
            "Silence_Alarm = 0x9D",
            "Simple_AV_Control = 0x94",
            "Sound_Switch = 0x79",
            "Supervision = 0x6C",
            "Switch_All = 0x27",
            "Switch_Binary = 0x25",
            "Switch_Color = 0x33",
            "Switch_Multilevel = 0x26",
            "Switch_Toggle_Binary = 0x28",
            "Switch_Toggle_Multilevel = 0x29",
            "Tariff_Config = 0x4A",
            "Tariff_Tbl_Monitor = 0x4B",
            "Thermostat_Fan_Mode = 0x44",
            "Thermostat_Fan_State = 0x45",
            "Thermostat_Mode = 0x40",
            "Thermostat_Operating_State = 0x42",
            "Thermostat_Setback = 0x47",
            "Thermostat_Setpoint = 0x43",
            "Time = 0x8A",
            "Time_Parameters = 0x8B",
            "Transport_Service = 0x55",
            "User_Code = 0x63",
            "Version = 0x86",
            "Wake_Up = 0x84",
            "Window_Covering = 0x6A",
            "ZIP = 0x23",
            "ZIP_6LoWPAN = 0x4F",
            "ZIP_Gateway = 0x5F",
            "ZIP_Naming = 0x68",
            "ZIP_ND = 0x58",
            "ZIP_Portal = 0x61",
            "ZWavePlus_Info = 0x5E",
            "Invalid = 0xFFFE",
        };
        static char *processed_names[_count];
        static bool initialized = false;
        if (!initialized)
        {
            for (size_t index = 0; index < _count; ++index)
            {
                size_t length = std::strcspn(raw_names[index], " =\t\n\r");
                processed_names[index] = new char[length + 1];
                strncpy(processed_names[index], raw_names[index], length);
                processed_names[index][length] = '\0';
            }
        }
        return processed_names;
    }
};
struct Alarm_Cmd
{
    enum _enumerated
    {
        Get = 0x04,
        Report = 0x05,
        Set = 0x06,
        Get_Supported = 0x07,
        Report_Supported = 0x08,
        Get_Supported_Event = 0x01,
        Report_Supported_Event = 0x02,
        Invalid = 0xFFFE
    };
    _enumerated _value;
    Alarm_Cmd() : _value(_enumerated::Invalid) {}
    Alarm_Cmd(_enumerated value) : _value(value) {}
    Alarm_Cmd(const int value)
    {
        if (_is_valid(value))
        {
            _value = static_cast<Alarm_Cmd::_enumerated>(value);
        }
        else
        {
            _value = _enumerated::Invalid;
        }
    }
    operator _enumerated() const { return _value; }
    int operator=(const int _val)
    {
        if (_is_valid(_val))
        {
            _value = static_cast<Alarm_Cmd::_enumerated>(_val);
        }
        else
        {
            _value = _enumerated::Invalid;
        }
        return _value;
    }
    static const bool _is_valid(int _var)
    {
        for (size_t index = 0; index < _count; ++index)
        {
            if (_values()[index] == _var)
            {
                return true;
            }
        }
        return false;
    }
    const char *_to_string() const
    {
        for (size_t index = 0; index < _count; ++index)
        {
            if (_values()[index] == _value)
            {
                return _names()[index];
            }
        }
        return NULL;
    }
    const char *_to_full_string() const
    {
        static std::string val("Alarm_Cmd");
        val = "Alarm_Cmd";
        val.append(".").append(_to_string());
        return val.c_str();
    }
    static const size_t _count = 8;
    static const int *_values()
    {
        static const int values[] = {
            (ignore_assign)Get = 0x04,
            (ignore_assign)Report = 0x05,
            (ignore_assign)Set = 0x06,
            (ignore_assign)Get_Supported = 0x07,
            (ignore_assign)Report_Supported = 0x08,
            (ignore_assign)Get_Supported_Event = 0x01,
            (ignore_assign)Report_Supported_Event = 0x02,
            (ignore_assign)Invalid = 0xFFFE,
        };
        return values;
    }
    static const char *const *_names()
    {
        static const char *const raw_names[] = {
            "Get = 0x04",
            "Report = 0x05",
            "Set = 0x06",
            "Get_Supported = 0x07",
            "Report_Supported = 0x08",
            "Get_Supported_Event = 0x01",
            "Report_Supported_Event = 0x02",
            "Invalid = 0xFFFE",
        };
        static char *processed_names[_count];
        static bool initialized = false;
        if (!initialized)
        {
            for (size_t index = 0; index < _count; ++index)
            {
                size_t length = std::strcspn(raw_names[index], " =\t\n\r");
                processed_names[index] = new char[length + 1];
                strncpy(processed_names[index], raw_names[index], length);
                processed_names[index][length] = '\0';
            }
        }
        return processed_names;
    }
};
struct NoOperation_Cmd
{
    enum _enumerated
    {
        Nop = 0x00,
        Invalid = 0xFFFE
    };
    _enumerated _value;
    NoOperation_Cmd() : _value(_enumerated::Invalid) {}
    NoOperation_Cmd(_enumerated value) : _value(value) {}
    NoOperation_Cmd(const int value)
    {
        if (_is_valid(value))
        {
            _value = static_cast<NoOperation_Cmd::_enumerated>(value);
        }
        else
        {
            _value = _enumerated::Invalid;
        }
    }
    operator _enumerated() const { return _value; }
    int operator=(const int _val)
    {
        if (_is_valid(_val))
        {
            _value = static_cast<NoOperation_Cmd::_enumerated>(_val);
        }
        else
        {
            _value = _enumerated::Invalid;
        }
        return _value;
    }
    static const bool _is_valid(int _var)
    {
        for (size_t index = 0; index < _count; ++index)
        {
            if (_values()[index] == _var)
            {
                return true;
            }
        }
        return false;
    }
    const char *_to_string() const
    {
        for (size_t index = 0; index < _count; ++index)
        {
            if (_values()[index] == _value)
            {
                return _names()[index];
            }
        }
        return NULL;
    }
    const char *_to_full_string() const
    {
        static std::string val("NoOperation_Cmd");
        val = "NoOperation_Cmd";
        val.append(".").append(_to_string());
        return val.c_str();
    }
    static const size_t _count = 2;
    static const int *_values()
    {
        static const int values[] = {
            (ignore_assign)Nop = 0x00,
            (ignore_assign)Invalid = 0xFFFE,
        };
        return values;
    }
    static const char *const *_names()
    {
        static const char *const raw_names[] = {
            "Nop = 0x00",
            "Invalid = 0xFFFE",
        };
        static char *processed_names[_count];
        static bool initialized = false;
        if (!initialized)
        {
            for (size_t index = 0; index < _count; ++index)
            {
                size_t length = std::strcspn(raw_names[index], " =\t\n\r");
                processed_names[index] = new char[length + 1];
                strncpy(processed_names[index], raw_names[index], length);
                processed_names[index][length] = '\0';
            }
        }
        return processed_names;
    }
};
