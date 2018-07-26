#include "command_classes/CommandClasses.h"
#include "command_classes/MultiInstance.h"
#include "command_classes/AVSimple.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"
#include <chrono>

#include "value_classes/ValueShort.h"
#include "value_classes/ValueList.h"

using namespace OpenZWave;

uint8 m_sequence;

enum SimpleAVCmd
{
	SimpleAVCmd_Set = 0x01
};

//-----------------------------------------------------------------------------
// <AVSimple::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool AVSimple::HandleMsg
(
	uint8 const* _data,
	uint32 const _length,
	uint32 const _instance
)
{
	return true;
}

//-----------------------------------------------------------------------------
// <AVSimple::SetValue>
// Set a value in the Z-Wave device
//-----------------------------------------------------------------------------
bool AVSimple::SetValue
(
	Value const& _value
)
{
	uint16 shortval;
	if (ValueID::ValueType_Short == _value.GetID().GetType())
	{
		ValueShort const* value = static_cast<ValueShort const*>(&_value);
		shortval = value->GetValue();
	}
	else if (ValueID::ValueType_List == _value.GetID().GetType())
	{
		ValueList const* value = static_cast<ValueList const*>(&_value);
		shortval = value->GetItem()->m_value;
	}
	else return false;

	uint8 instance = _value.GetID().GetInstance();
	Msg* msg = new Msg("SimpleAVCmd_Set", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true);
	msg->SetInstance(this, instance);
	msg->Append(GetNodeId());
	msg->Append(8); // Length
	msg->Append(GetCommandClassId());
	msg->Append(SimpleAVCmd_Set);
	msg->Append(m_sequence++); // Crutch
	msg->Append(0);
	msg->Append(0);
	msg->Append(0);
	msg->Append(shortval >> 8);
	msg->Append(shortval & 0xff);
	msg->Append(GetDriver()->GetTransmitOptions());
	GetDriver()->SendMsg(msg, Driver::MsgQueue_Send);

	if (m_sequence == 255)
		m_sequence = 0;

	return true;
}

//-----------------------------------------------------------------------------
// <AVSimple::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void AVSimple::CreateVars
(
	uint8 const _instance
)
{
	if (Node* node = GetNodeUnsafe())
	{
		// Create list value
		vector<ValueList::Item> items;
		vector<AVSimpleCommand> commands = AVSimpleCommand::GetCommands();
		vector<AVSimpleCommand>::iterator iterator;
		string helpList = "Possible values: \n";
		string helpNumeric = "Possible values: \n";
		for (iterator = commands.begin(); iterator != commands.end(); iterator++)
		{
			AVSimpleCommand command = *iterator;
			if (command.GetVersion() <= GetVersion())
			{
				ValueList::Item item;
				item.m_value = command.GetCode();
				item.m_label = command.GetName();
				items.push_back(item);

				// ValueList - create command description for help
				string codeStr = "ZWave command number: " + command.GetCode();
				string descriptionFull = "";
				if (command.GetDescription() == "")
				{
					descriptionFull = command.GetName() + " (" + codeStr + ")\n";
				}
				else
				{
					descriptionFull = command.GetName() + " (" + command.GetDescription() + "; " + codeStr + ")\n";
				}
				helpList +=  descriptionFull;

				// ValueShort - create command description for help 
				if (command.GetDescription() == "")
				{
					descriptionFull = command.GetCode() + " (" + command.GetName() + ")\n";
				}
				else
				{
					descriptionFull = command.GetCode() + " (" + command.GetName() + "; " + command.GetDescription() + ")\n";
				}
				helpNumeric += descriptionFull;
			}
		}

		node->CreateValueList(ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "OutputAVCommand_" + _instance, "", false, true, 2, items, 0, 0, helpList);
		
		// Create a similar numeric value
		node->CreateValueShort(ValueID::ValueGenre_User, GetCommandClassId(), _instance, 0, "OutputAVCommandNumber_" + _instance, "", false, true, 0, 0, helpNumeric);
	}

}

//-----------------------------------------------------------------------------
// <AVSimpleCommand::AVSimpleCommand>
// AVSimpleCommand constructor
//-----------------------------------------------------------------------------
AVSimpleCommand::AVSimpleCommand
(
	uint16 const _code,
	string _name,
	string _description,
	uint16 const _version
) 
{
	m_code = _code;
	m_name = _name;
	m_description = _description;
	m_version = _version;
}

//-----------------------------------------------------------------------------
// <AVSimpleCommand::GetCode>
// Get code of ZWave AV command
//-----------------------------------------------------------------------------
uint16 AVSimpleCommand::GetCode() 
{
	return m_code;
}

//-----------------------------------------------------------------------------
// <AVSimpleCommand::GetVersion>
// Get version of AV simple command class of command
//-----------------------------------------------------------------------------
uint16 AVSimpleCommand::GetVersion()
{
	return m_version;
}

//-----------------------------------------------------------------------------
// <AVSimpleCommand::GetName>
// Get human-friendly name of AV command
//-----------------------------------------------------------------------------
string AVSimpleCommand::GetName()
{
	return m_name;
}

//-----------------------------------------------------------------------------
// <AVSimpleCommand::GetDescription>
// Get human-friendly description of AV command
//-----------------------------------------------------------------------------
string AVSimpleCommand::GetDescription()
{
	return m_description;
}

//-----------------------------------------------------------------------------
// <AVSimpleCommand::GetCommands>
// Get all available AV commands
//-----------------------------------------------------------------------------
vector<AVSimpleCommand> AVSimpleCommand::GetCommands() 
{
	if (m_commands.size() == 0)
	{
		// Generated code
		m_commands.push_back(AVSimpleCommand(1, "Mute", "", 1));
		m_commands.push_back(AVSimpleCommand(2, "Volume Down", "Level Down", 1));
		m_commands.push_back(AVSimpleCommand(3, "Volume Up", "Level Up", 1));
		m_commands.push_back(AVSimpleCommand(4, "Channel Up", "Program Up", 1));
		m_commands.push_back(AVSimpleCommand(5, "Channel Down", "Program Down", 1));
		m_commands.push_back(AVSimpleCommand(6, "0", "Preset 10", 1));
		m_commands.push_back(AVSimpleCommand(7, "1", "Preset 1", 1));
		m_commands.push_back(AVSimpleCommand(8, "2", "Preset 2", 1));
		m_commands.push_back(AVSimpleCommand(9, "3", "Preset 3", 1));
		m_commands.push_back(AVSimpleCommand(10, "4", "Preset 4", 1));
		m_commands.push_back(AVSimpleCommand(11, "5", "Preset 5", 1));
		m_commands.push_back(AVSimpleCommand(12, "6", "Preset 6", 1));
		m_commands.push_back(AVSimpleCommand(13, "7", "Preset 7", 1));
		m_commands.push_back(AVSimpleCommand(14, "8", "Preset 8", 1));
		m_commands.push_back(AVSimpleCommand(15, "9", "Preset 9", 1));
		m_commands.push_back(AVSimpleCommand(16, "Last Channel", "Recall, Previous Channel (WMC)", 1));
		m_commands.push_back(AVSimpleCommand(17, "Display", "Info", 1));
		m_commands.push_back(AVSimpleCommand(18, "Favorite Channel", "Favorite", 1));
		m_commands.push_back(AVSimpleCommand(19, "Play", "", 1));
		m_commands.push_back(AVSimpleCommand(20, "Stop", "", 1));
		m_commands.push_back(AVSimpleCommand(21, "Pause", "Still", 1));
		m_commands.push_back(AVSimpleCommand(22, "Fast Forward", "Search Forward", 1));
		m_commands.push_back(AVSimpleCommand(23, "Rewind", "Search Reverse", 1));
		m_commands.push_back(AVSimpleCommand(24, "Instant Replay", "Replay", 1));
		m_commands.push_back(AVSimpleCommand(25, "Record", "", 1));
		m_commands.push_back(AVSimpleCommand(26, "AC3", "Dolby Digital", 1));
		m_commands.push_back(AVSimpleCommand(27, "PVR Menu", "Tivo", 1));
		m_commands.push_back(AVSimpleCommand(28, "Guide", "EPG", 1));
		m_commands.push_back(AVSimpleCommand(29, "Menu", "Settings", 1));
		m_commands.push_back(AVSimpleCommand(30, "Menu Up", "Adjust Up", 1));
		m_commands.push_back(AVSimpleCommand(31, "Menu Down", "Adjust Down", 1));
		m_commands.push_back(AVSimpleCommand(32, "Menu Left", "Cursor Left", 1));
		m_commands.push_back(AVSimpleCommand(33, "Menu Right", "Cursor Right", 1));
		m_commands.push_back(AVSimpleCommand(34, "Page Up", "", 1));
		m_commands.push_back(AVSimpleCommand(35, "Page Down", "", 1));
		m_commands.push_back(AVSimpleCommand(36, "Select", "OK", 1));
		m_commands.push_back(AVSimpleCommand(37, "Exit", "", 1));
		m_commands.push_back(AVSimpleCommand(38, "Input", "Input Select", 1));
		m_commands.push_back(AVSimpleCommand(39, "Power", "Standby", 1));
		m_commands.push_back(AVSimpleCommand(40, "Enter Channel", "Channel Enter", 1));
		m_commands.push_back(AVSimpleCommand(41, "10", "", 1));
		m_commands.push_back(AVSimpleCommand(42, "11", "", 1));
		m_commands.push_back(AVSimpleCommand(43, "12", "", 1));
		m_commands.push_back(AVSimpleCommand(44, "13", "", 1));
		m_commands.push_back(AVSimpleCommand(45, "14", "", 1));
		m_commands.push_back(AVSimpleCommand(46, "15", "", 1));
		m_commands.push_back(AVSimpleCommand(47, "16", "", 1));
		m_commands.push_back(AVSimpleCommand(48, "10", "10+", 1));
		m_commands.push_back(AVSimpleCommand(49, "20", "20+", 1));
		m_commands.push_back(AVSimpleCommand(50, "100", "", 1));
		m_commands.push_back(AVSimpleCommand(51, "-/--", "", 1));
		m_commands.push_back(AVSimpleCommand(52, "3-CH", "", 1));
		m_commands.push_back(AVSimpleCommand(53, "3D", "Simulated Stereo", 1));
		m_commands.push_back(AVSimpleCommand(54, "6-CH Input", "6 Channel", 1));
		m_commands.push_back(AVSimpleCommand(55, "A", "", 1));
		m_commands.push_back(AVSimpleCommand(56, "Add", "Write", 1));
		m_commands.push_back(AVSimpleCommand(57, "Alarm", "", 1));
		m_commands.push_back(AVSimpleCommand(58, "AM", "", 1));
		m_commands.push_back(AVSimpleCommand(59, "Analog", "", 1));
		m_commands.push_back(AVSimpleCommand(60, "Angle", "", 1));
		m_commands.push_back(AVSimpleCommand(61, "Antenna", "External", 1));
		m_commands.push_back(AVSimpleCommand(62, "Antenna East", "", 1));
		m_commands.push_back(AVSimpleCommand(63, "Antenna West", "", 1));
		m_commands.push_back(AVSimpleCommand(64, "Aspect", "Size", 1));
		m_commands.push_back(AVSimpleCommand(65, "Audio 1", "Audio", 1));
		m_commands.push_back(AVSimpleCommand(66, "Audio 2", "", 1));
		m_commands.push_back(AVSimpleCommand(67, "Audio 3", "", 1));
		m_commands.push_back(AVSimpleCommand(68, "Audio Dubbing", "", 1));
		m_commands.push_back(AVSimpleCommand(69, "Audio Level Down", "", 1));
		m_commands.push_back(AVSimpleCommand(70, "Audio Level Up", "", 1));
		m_commands.push_back(AVSimpleCommand(71, "Auto/Manual", "", 1));
		m_commands.push_back(AVSimpleCommand(72, "Aux 1", "Aux", 1));
		m_commands.push_back(AVSimpleCommand(73, "Aux 2", "", 1));
		m_commands.push_back(AVSimpleCommand(74, "B", "", 1));
		m_commands.push_back(AVSimpleCommand(75, "Back", "Previous Screen", 1));
		m_commands.push_back(AVSimpleCommand(76, "Background", "Backlight", 1));
		m_commands.push_back(AVSimpleCommand(77, "Balance", "", 1));
		m_commands.push_back(AVSimpleCommand(78, "Balance Left", "", 1));
		m_commands.push_back(AVSimpleCommand(79, "Balance Right", "", 1));
		m_commands.push_back(AVSimpleCommand(80, "Band", "FM/AM", 1));
		m_commands.push_back(AVSimpleCommand(81, "Bandwidth", "Wide/Narrow", 1));
		m_commands.push_back(AVSimpleCommand(82, "Bass", "", 1));
		m_commands.push_back(AVSimpleCommand(83, "Bass Down", "", 1));
		m_commands.push_back(AVSimpleCommand(84, "Bass Up", "", 1));
		m_commands.push_back(AVSimpleCommand(85, "Blank", "", 1));
		m_commands.push_back(AVSimpleCommand(86, "Breeze Mode", "", 1));
		m_commands.push_back(AVSimpleCommand(87, "Bright", "Brighten", 1));
		m_commands.push_back(AVSimpleCommand(88, "Brightness", "", 1));
		m_commands.push_back(AVSimpleCommand(89, "Brightness Down", "", 1));
		m_commands.push_back(AVSimpleCommand(90, "Brightness Up", "", 1));
		m_commands.push_back(AVSimpleCommand(91, "Buy", "", 1));
		m_commands.push_back(AVSimpleCommand(92, "C", "", 1));
		m_commands.push_back(AVSimpleCommand(93, "Camera", "", 1));
		m_commands.push_back(AVSimpleCommand(94, "Category Down", "", 1));
		m_commands.push_back(AVSimpleCommand(95, "Category Up", "", 1));
		m_commands.push_back(AVSimpleCommand(96, "Center", "", 1));
		m_commands.push_back(AVSimpleCommand(97, "Center Down", "Center Volume Down", 1));
		m_commands.push_back(AVSimpleCommand(98, "Center Mode", "", 1));
		m_commands.push_back(AVSimpleCommand(99, "Center Up", "Center Volume Up", 1));
		m_commands.push_back(AVSimpleCommand(100, "Channel/Program", "C/P", 1));
		m_commands.push_back(AVSimpleCommand(101, "Clear", "Cancel", 1));
		m_commands.push_back(AVSimpleCommand(102, "Close", "", 1));
		m_commands.push_back(AVSimpleCommand(103, "Closed Caption", "CC", 1));
		m_commands.push_back(AVSimpleCommand(104, "Cold", "A/C", 1));
		m_commands.push_back(AVSimpleCommand(105, "Color", "", 1));
		m_commands.push_back(AVSimpleCommand(106, "Color Down", "", 1));
		m_commands.push_back(AVSimpleCommand(107, "Color Up", "", 1));
		m_commands.push_back(AVSimpleCommand(108, "Component 1", "RGB 1", 1));
		m_commands.push_back(AVSimpleCommand(109, "Component 2", "RGB 2", 1));
		m_commands.push_back(AVSimpleCommand(110, "Component 3", "", 1));
		m_commands.push_back(AVSimpleCommand(111, "Concert", "", 1));
		m_commands.push_back(AVSimpleCommand(112, "Confirm", "Check", 1));
		m_commands.push_back(AVSimpleCommand(113, "Continue", "Continuous", 1));
		m_commands.push_back(AVSimpleCommand(114, "Contrast", "", 1));
		m_commands.push_back(AVSimpleCommand(115, "Contrast Down", "", 1));
		m_commands.push_back(AVSimpleCommand(116, "Contrast Up", "", 1));
		m_commands.push_back(AVSimpleCommand(117, "Counter", "", 1));
		m_commands.push_back(AVSimpleCommand(118, "Counter Reset", "", 1));
		m_commands.push_back(AVSimpleCommand(119, "D", "", 1));
		m_commands.push_back(AVSimpleCommand(120, "Day Down", "", 1));
		m_commands.push_back(AVSimpleCommand(121, "Day Up", "", 1));
		m_commands.push_back(AVSimpleCommand(122, "Delay", "", 1));
		m_commands.push_back(AVSimpleCommand(123, "Delay Down", "", 1));
		m_commands.push_back(AVSimpleCommand(124, "Delay Up", "", 1));
		m_commands.push_back(AVSimpleCommand(125, "Delete", "Erase", 1));
		m_commands.push_back(AVSimpleCommand(126, "Delimiter", "Sub-Channel", 1));
		m_commands.push_back(AVSimpleCommand(127, "Digest", "", 1));
		m_commands.push_back(AVSimpleCommand(128, "Digital", "", 1));
		m_commands.push_back(AVSimpleCommand(129, "Dim", "Dimmer", 1));
		m_commands.push_back(AVSimpleCommand(130, "Direct", "", 1));
		m_commands.push_back(AVSimpleCommand(131, "Disarm", "", 1));
		m_commands.push_back(AVSimpleCommand(132, "Disc", "", 1));
		m_commands.push_back(AVSimpleCommand(133, "Disc 1", "", 1));
		m_commands.push_back(AVSimpleCommand(134, "Disc 2", "", 1));
		m_commands.push_back(AVSimpleCommand(135, "Disc 3", "", 1));
		m_commands.push_back(AVSimpleCommand(136, "Disc 4", "", 1));
		m_commands.push_back(AVSimpleCommand(137, "Disc 5", "", 1));
		m_commands.push_back(AVSimpleCommand(138, "Disc 6", "", 1));
		m_commands.push_back(AVSimpleCommand(139, "Disc Down", "", 1));
		m_commands.push_back(AVSimpleCommand(140, "Disc Up", "", 1));
		m_commands.push_back(AVSimpleCommand(141, "Disco", "", 1));
		m_commands.push_back(AVSimpleCommand(142, "Edit", "", 1));
		m_commands.push_back(AVSimpleCommand(143, "Effect Down", "", 1));
		m_commands.push_back(AVSimpleCommand(144, "Effect Up", "", 1));
		m_commands.push_back(AVSimpleCommand(145, "Eject", "Open/Close", 1));
		m_commands.push_back(AVSimpleCommand(146, "End", "", 1));
		m_commands.push_back(AVSimpleCommand(147, "EQ", "Equalizer", 1));
		m_commands.push_back(AVSimpleCommand(148, "Fader", "", 1));
		m_commands.push_back(AVSimpleCommand(149, "Fan", "", 1));
		m_commands.push_back(AVSimpleCommand(150, "Fan High", "", 1));
		m_commands.push_back(AVSimpleCommand(151, "Fan Low", "", 1));
		m_commands.push_back(AVSimpleCommand(152, "Fan Medium", "", 1));
		m_commands.push_back(AVSimpleCommand(153, "Fan Speed", "", 1));
		m_commands.push_back(AVSimpleCommand(154, "Fastext Blue", "", 1));
		m_commands.push_back(AVSimpleCommand(155, "Fastext Green", "", 1));
		m_commands.push_back(AVSimpleCommand(156, "Fastext Purple", "", 1));
		m_commands.push_back(AVSimpleCommand(157, "Fastext Red", "", 1));
		m_commands.push_back(AVSimpleCommand(158, "Fastext White", "", 1));
		m_commands.push_back(AVSimpleCommand(159, "Fastext Yellow", "", 1));
		m_commands.push_back(AVSimpleCommand(160, "Favorite Channel Down", "", 1));
		m_commands.push_back(AVSimpleCommand(161, "Favorite Channel Up", "", 1));
		m_commands.push_back(AVSimpleCommand(162, "Finalize", "", 1));
		m_commands.push_back(AVSimpleCommand(163, "Fine Tune", "", 1));
		m_commands.push_back(AVSimpleCommand(164, "Flat", "", 1));
		m_commands.push_back(AVSimpleCommand(165, "FM", "", 1));
		m_commands.push_back(AVSimpleCommand(166, "Focus Down", "", 1));
		m_commands.push_back(AVSimpleCommand(167, "Focus Up", "", 1));
		m_commands.push_back(AVSimpleCommand(168, "Freeze", "", 1));
		m_commands.push_back(AVSimpleCommand(169, "Front", "", 1));
		m_commands.push_back(AVSimpleCommand(170, "Game", "", 1));
		m_commands.push_back(AVSimpleCommand(171, "GoTo", "Index Search", 1));
		m_commands.push_back(AVSimpleCommand(172, "Hall", "", 1));
		m_commands.push_back(AVSimpleCommand(173, "Heat", "", 1));
		m_commands.push_back(AVSimpleCommand(174, "Help", "", 1));
		m_commands.push_back(AVSimpleCommand(175, "Home", "", 1));
		m_commands.push_back(AVSimpleCommand(176, "Index", "VISS", 1));
		m_commands.push_back(AVSimpleCommand(177, "Index Forward", "", 1));
		m_commands.push_back(AVSimpleCommand(178, "Index Reverse", "", 1));
		m_commands.push_back(AVSimpleCommand(179, "Interactive", "Planner", 1));
		m_commands.push_back(AVSimpleCommand(180, "Intro Scan", "", 1));
		m_commands.push_back(AVSimpleCommand(181, "Jazz", "", 1));
		m_commands.push_back(AVSimpleCommand(182, "Karaoke", "", 1));
		m_commands.push_back(AVSimpleCommand(183, "Keystone", "", 1));
		m_commands.push_back(AVSimpleCommand(184, "Keystone Down", "", 1));
		m_commands.push_back(AVSimpleCommand(185, "Keystone Up", "", 1));
		m_commands.push_back(AVSimpleCommand(186, "Language", "SAP", 1));
		m_commands.push_back(AVSimpleCommand(187, "Left Click", "", 1));
		m_commands.push_back(AVSimpleCommand(188, "Level", "Volume", 1));
		m_commands.push_back(AVSimpleCommand(189, "Light", "Lamp", 1));
		m_commands.push_back(AVSimpleCommand(190, "List", "My Shows", 1));
		m_commands.push_back(AVSimpleCommand(191, "Live TV", "Return to Live", 1));
		m_commands.push_back(AVSimpleCommand(192, "Local/Dx", "", 1));
		m_commands.push_back(AVSimpleCommand(193, "Loudness", "", 1));
		m_commands.push_back(AVSimpleCommand(194, "Mail", "Email", 1));
		m_commands.push_back(AVSimpleCommand(195, "Mark", "Bookmark", 1));
		m_commands.push_back(AVSimpleCommand(196, "Memory Recall", "", 1));
		m_commands.push_back(AVSimpleCommand(197, "Monitor", "Tape Monitor", 1));
		m_commands.push_back(AVSimpleCommand(198, "Movie", "", 1));
		m_commands.push_back(AVSimpleCommand(199, "Multi Room", "", 1));
		m_commands.push_back(AVSimpleCommand(200, "Music", "TV/Radio, My Music (WMC)", 1));
		m_commands.push_back(AVSimpleCommand(201, "Music Scan", "Memory Scan", 1));
		m_commands.push_back(AVSimpleCommand(202, "Natural", "", 1));
		m_commands.push_back(AVSimpleCommand(203, "Night", "", 1));
		m_commands.push_back(AVSimpleCommand(204, "Noise Reduction", "Dolby NR", 1));
		m_commands.push_back(AVSimpleCommand(205, "Normalize", "Personal Preference", 1));
		m_commands.push_back(AVSimpleCommand(206, "Discrete input Cable", "CATV", 1));
		m_commands.push_back(AVSimpleCommand(207, "Discrete input CD 1", "CD", 1));
		m_commands.push_back(AVSimpleCommand(208, "Discrete input CD 2", "CDR", 1));
		m_commands.push_back(AVSimpleCommand(209, "Discrete input CDR", "Compact Disc Recorder", 1));
		m_commands.push_back(AVSimpleCommand(210, "Discrete input DAT", "Digital Audio Tape", 1));
		m_commands.push_back(AVSimpleCommand(211, "Discrete input DVD", "Digital Video Disk", 1));
		m_commands.push_back(AVSimpleCommand(212, "Discrete input DVI", "Digital Video Interface", 1));
		m_commands.push_back(AVSimpleCommand(213, "Discrete input HDTV", "", 1));
		m_commands.push_back(AVSimpleCommand(214, "Discrete input LD", "Laser Disc", 1));
		m_commands.push_back(AVSimpleCommand(215, "Discrete input MD", "Mini Disc", 1));
		m_commands.push_back(AVSimpleCommand(216, "Discrete input PC", "Personal Computer", 1));
		m_commands.push_back(AVSimpleCommand(217, "Discrete input PVR", "Personal Video Recorder", 1));
		m_commands.push_back(AVSimpleCommand(218, "Discrete input TV", "", 1));
		m_commands.push_back(AVSimpleCommand(219, "Discrete input TV/VCR", "TV/DVD", 1));
		m_commands.push_back(AVSimpleCommand(220, "Discrete input VCR", "", 1));
		m_commands.push_back(AVSimpleCommand(221, "One Touch Playback", "OTPB", 1));
		m_commands.push_back(AVSimpleCommand(222, "One Touch Record", "OTR", 1));
		m_commands.push_back(AVSimpleCommand(223, "Open", "", 1));
		m_commands.push_back(AVSimpleCommand(224, "Optical", "", 1));
		m_commands.push_back(AVSimpleCommand(225, "Options", "", 1));
		m_commands.push_back(AVSimpleCommand(226, "Orchestra", "", 1));
		m_commands.push_back(AVSimpleCommand(227, "PAL/NTSC", "System Select", 1));
		m_commands.push_back(AVSimpleCommand(228, "Parental Lock", "Parental Control", 1));
		m_commands.push_back(AVSimpleCommand(229, "PBC", "Playback Control", 1));
		m_commands.push_back(AVSimpleCommand(230, "Phono", "", 1));
		m_commands.push_back(AVSimpleCommand(231, "Photos", "Pictures, My Pictures (WMC)", 1));
		m_commands.push_back(AVSimpleCommand(232, "Picture Menu", "Picture Adjust", 1));
		m_commands.push_back(AVSimpleCommand(233, "Picture Mode", "Smart Picture", 1));
		m_commands.push_back(AVSimpleCommand(234, "Picture Mute", "", 1));
		m_commands.push_back(AVSimpleCommand(235, "PIP Channel Down", "", 1));
		m_commands.push_back(AVSimpleCommand(236, "PIP Channel Up", "", 1));
		m_commands.push_back(AVSimpleCommand(237, "PIP Freeze", "", 1));
		m_commands.push_back(AVSimpleCommand(238, "PIP Input", "PIP Mode", 1));
		m_commands.push_back(AVSimpleCommand(239, "PIP Move", "PIP Position", 1));
		m_commands.push_back(AVSimpleCommand(240, "PIP Off", "", 1));
		m_commands.push_back(AVSimpleCommand(241, "PIP On", "PIP", 1));
		m_commands.push_back(AVSimpleCommand(242, "PIP Size", "", 1));
		m_commands.push_back(AVSimpleCommand(243, "PIP Split", "Multi Screen", 1));
		m_commands.push_back(AVSimpleCommand(244, "PIP Swap", "PIP Exchange", 1));
		m_commands.push_back(AVSimpleCommand(245, "Play Mode", "", 1));
		m_commands.push_back(AVSimpleCommand(246, "Play Reverse", "", 1));
		m_commands.push_back(AVSimpleCommand(247, "Power Off", "", 1));
		m_commands.push_back(AVSimpleCommand(248, "Power On", "", 1));
		m_commands.push_back(AVSimpleCommand(249, "PPV", "Pay Per View", 1));
		m_commands.push_back(AVSimpleCommand(250, "Preset", "", 1));
		m_commands.push_back(AVSimpleCommand(251, "Program", "Program Memory", 1));
		m_commands.push_back(AVSimpleCommand(252, "Progressive Scan", "Progressive", 1));
		m_commands.push_back(AVSimpleCommand(253, "ProLogic", "Dolby Prologic", 1));
		m_commands.push_back(AVSimpleCommand(254, "PTY", "Audio Program Type", 1));
		m_commands.push_back(AVSimpleCommand(255, "Quick Skip", "Commercial Skip", 1));
		m_commands.push_back(AVSimpleCommand(256, "Random", "Shuffle", 1));
		m_commands.push_back(AVSimpleCommand(257, "RDS", "Radio Data System", 1));
		m_commands.push_back(AVSimpleCommand(258, "Rear", "", 1));
		m_commands.push_back(AVSimpleCommand(259, "Rear Volume Down", "", 1));
		m_commands.push_back(AVSimpleCommand(260, "Rear Volume Up", "", 1));
		m_commands.push_back(AVSimpleCommand(261, "Record Mute", "", 1));
		m_commands.push_back(AVSimpleCommand(262, "Record Pause", "", 1));
		m_commands.push_back(AVSimpleCommand(263, "Repeat", "", 1));
		m_commands.push_back(AVSimpleCommand(264, "Repeat A-B", "", 1));
		m_commands.push_back(AVSimpleCommand(265, "Resume", "", 1));
		m_commands.push_back(AVSimpleCommand(266, "RGB", "Red Green Blue Component Video", 1));
		m_commands.push_back(AVSimpleCommand(267, "Right Click", "", 1));
		m_commands.push_back(AVSimpleCommand(268, "Rock", "", 1));
		m_commands.push_back(AVSimpleCommand(269, "Rotate Left", "", 1));
		m_commands.push_back(AVSimpleCommand(270, "Rotate Right", "", 1));
		m_commands.push_back(AVSimpleCommand(271, "SAT", "Sky", 1));
		m_commands.push_back(AVSimpleCommand(272, "Scan", "Channel Scan", 1));
		m_commands.push_back(AVSimpleCommand(273, "Scart", "", 1));
		m_commands.push_back(AVSimpleCommand(274, "Scene", "", 1));
		m_commands.push_back(AVSimpleCommand(275, "Scroll", "", 1));
		m_commands.push_back(AVSimpleCommand(276, "Services", "", 1));
		m_commands.push_back(AVSimpleCommand(277, "Setup Menu", "Setup", 1));
		m_commands.push_back(AVSimpleCommand(278, "Sharp", "", 1));
		m_commands.push_back(AVSimpleCommand(279, "Sharpness", "", 1));
		m_commands.push_back(AVSimpleCommand(280, "Sharpness Down", "", 1));
		m_commands.push_back(AVSimpleCommand(281, "Sharpness Up", "", 1));
		m_commands.push_back(AVSimpleCommand(282, "Side A/B", "", 1));
		m_commands.push_back(AVSimpleCommand(283, "Skip Forward", "Next", 1));
		m_commands.push_back(AVSimpleCommand(284, "Skip Reverse", "Previous", 1));
		m_commands.push_back(AVSimpleCommand(285, "Sleep", "Off Timer", 1));
		m_commands.push_back(AVSimpleCommand(286, "Slow", "", 1));
		m_commands.push_back(AVSimpleCommand(287, "Slow Forward", "", 1));
		m_commands.push_back(AVSimpleCommand(288, "Slow Reverse", "", 1));
		m_commands.push_back(AVSimpleCommand(289, "Sound Menu", "Audio Menu", 1));
		m_commands.push_back(AVSimpleCommand(290, "Sound Mode", "Smart Sound", 1));
		m_commands.push_back(AVSimpleCommand(291, "Speed", "Record Speed", 1));
		m_commands.push_back(AVSimpleCommand(292, "Speed Down", "", 1));
		m_commands.push_back(AVSimpleCommand(293, "Speed Up", "", 1));
		m_commands.push_back(AVSimpleCommand(294, "Sports", "Digital Surround Processing", 1));
		m_commands.push_back(AVSimpleCommand(295, "Stadium", "", 1));
		m_commands.push_back(AVSimpleCommand(296, "Start", "", 1));
		m_commands.push_back(AVSimpleCommand(297, "Start ID Erase", "Erase", 1));
		m_commands.push_back(AVSimpleCommand(298, "Start ID Renumber", "Renumber", 1));
		m_commands.push_back(AVSimpleCommand(299, "Start ID Write", "Write", 1));
		m_commands.push_back(AVSimpleCommand(300, "Step", "", 1));
		m_commands.push_back(AVSimpleCommand(301, "Stereo/Mono", "L/R", 1));
		m_commands.push_back(AVSimpleCommand(302, "Still Forward", "Frame Advance", 1));
		m_commands.push_back(AVSimpleCommand(303, "Still Reverse", "Frame Reverse", 1));
		m_commands.push_back(AVSimpleCommand(304, "Subtitle", "Subtitle On-Off", 1));
		m_commands.push_back(AVSimpleCommand(305, "Subwoofer Down", "", 1));
		m_commands.push_back(AVSimpleCommand(306, "Subwoofer Up", "", 1));
		m_commands.push_back(AVSimpleCommand(307, "Super Bass", "Bass Boost", 1));
		m_commands.push_back(AVSimpleCommand(308, "Surround", "", 1));
		m_commands.push_back(AVSimpleCommand(309, "Surround Mode", "Sound Field", 1));
		m_commands.push_back(AVSimpleCommand(310, "S-Video", "", 1));
		m_commands.push_back(AVSimpleCommand(311, "Sweep", "Oscillate", 1));
		m_commands.push_back(AVSimpleCommand(312, "Synchro Record", "CD Synchro", 1));
		m_commands.push_back(AVSimpleCommand(313, "Tape 1", "Deck 1", 1));
		m_commands.push_back(AVSimpleCommand(314, "Tape 1-2", "Deck 1-2", 1));
		m_commands.push_back(AVSimpleCommand(315, "Tape 2", "Deck 2", 1));
		m_commands.push_back(AVSimpleCommand(316, "Temperature Down", "", 1));
		m_commands.push_back(AVSimpleCommand(317, "Temperature Up", "", 1));
		m_commands.push_back(AVSimpleCommand(318, "Test Tone", "", 1));
		m_commands.push_back(AVSimpleCommand(319, "Text", "Teletext", 1));
		m_commands.push_back(AVSimpleCommand(320, "Text Expand", "", 1));
		m_commands.push_back(AVSimpleCommand(321, "Text Hold", "", 1));
		m_commands.push_back(AVSimpleCommand(322, "Text Index", "", 1));
		m_commands.push_back(AVSimpleCommand(323, "Text Mix", "", 1));
		m_commands.push_back(AVSimpleCommand(324, "Text Off", "", 1));
		m_commands.push_back(AVSimpleCommand(325, "Text Reveal", "", 1));
		m_commands.push_back(AVSimpleCommand(326, "Text Subpage", "", 1));
		m_commands.push_back(AVSimpleCommand(327, "Text Timed Page", "", 1));
		m_commands.push_back(AVSimpleCommand(328, "Text Update", "Text Cancel", 1));
		m_commands.push_back(AVSimpleCommand(329, "Theater", "Cinema EQ", 1));
		m_commands.push_back(AVSimpleCommand(330, "Theme", "Category Select", 1));
		m_commands.push_back(AVSimpleCommand(331, "Thumbs Down", "", 1));
		m_commands.push_back(AVSimpleCommand(332, "Thumbs Up", "", 1));
		m_commands.push_back(AVSimpleCommand(333, "Tilt Down", "", 1));
		m_commands.push_back(AVSimpleCommand(334, "Tilt Up", "", 1));
		m_commands.push_back(AVSimpleCommand(335, "Time", "Clock", 1));
		m_commands.push_back(AVSimpleCommand(336, "Timer", "", 1));
		m_commands.push_back(AVSimpleCommand(337, "Timer Down", "", 1));
		m_commands.push_back(AVSimpleCommand(338, "Timer Up", "", 1));
		m_commands.push_back(AVSimpleCommand(339, "Tint", "", 1));
		m_commands.push_back(AVSimpleCommand(340, "Tint Down", "", 1));
		m_commands.push_back(AVSimpleCommand(341, "Tint Up", "", 1));
		m_commands.push_back(AVSimpleCommand(342, "Title", "Top Menu", 1));
		m_commands.push_back(AVSimpleCommand(343, "Track", "Chapter", 1));
		m_commands.push_back(AVSimpleCommand(344, "Tracking", "", 1));
		m_commands.push_back(AVSimpleCommand(345, "Tracking Down", "", 1));
		m_commands.push_back(AVSimpleCommand(346, "Tracking Up", "", 1));
		m_commands.push_back(AVSimpleCommand(347, "Treble", "", 1));
		m_commands.push_back(AVSimpleCommand(348, "Treble Down", "", 1));
		m_commands.push_back(AVSimpleCommand(349, "Treble Up", "", 1));
		m_commands.push_back(AVSimpleCommand(350, "Tune Down", "Audio Tune Down", 1));
		m_commands.push_back(AVSimpleCommand(351, "Tune Up", "Audio Tune Up", 1));
		m_commands.push_back(AVSimpleCommand(352, "Tuner", "", 1));
		m_commands.push_back(AVSimpleCommand(353, "VCR Plus+", "Showview", 1));
		m_commands.push_back(AVSimpleCommand(354, "Video 1", "A/V 1", 1));
		m_commands.push_back(AVSimpleCommand(355, "Video 2", "A/V 2", 1));
		m_commands.push_back(AVSimpleCommand(356, "Video 3", "A/V 3", 1));
		m_commands.push_back(AVSimpleCommand(357, "Video 4", "A/V 4", 1));
		m_commands.push_back(AVSimpleCommand(358, "Video 5", "A/V 5", 1));
		m_commands.push_back(AVSimpleCommand(359, "View", "", 1));
		m_commands.push_back(AVSimpleCommand(360, "Voice", "Vocals", 1));
		m_commands.push_back(AVSimpleCommand(361, "Zoom", "Magnify", 1));
		m_commands.push_back(AVSimpleCommand(362, "Zoom In", "Zoom Up", 1));
		m_commands.push_back(AVSimpleCommand(363, "Zoom Out", "Zoom Down", 1));
		m_commands.push_back(AVSimpleCommand(364, "eHome", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommand(365, "Details", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommand(366, "DVD Menu", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommand(367, "My TV", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommand(368, "Recorded TV", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommand(369, "My Videos", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommand(370, "DVD Angle", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommand(371, "DVD Audio", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommand(372, "DVD Subtitle", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommand(373, "Radio", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommand(374, "#", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommand(375, "*", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommand(376, "OEM 1", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommand(377, "OEM 2", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommand(378, "Info", "Used to request information", 3));
		m_commands.push_back(AVSimpleCommand(379, "CAPS NUM", "Switch between numeric and alpha (Shift)", 3));
		m_commands.push_back(AVSimpleCommand(380, "TV MODE", "Cycles through video output modes/resolutions", 3));
		m_commands.push_back(AVSimpleCommand(381, "SOURCE", "Displays the possible sources for the playback. [NFS, ext., USB, UPnP,…]", 3));
		m_commands.push_back(AVSimpleCommand(382, "FILEMODE", "File manipulation. Add/remove to list, create folder, rename file,…", 3));
		m_commands.push_back(AVSimpleCommand(383, "Time Seek", "This seeks to time position. Used for DVD/CD/others", 3));
		m_commands.push_back(AVSimpleCommand(384, "Mouse enable", "Mouse pointer enable", 4));
		m_commands.push_back(AVSimpleCommand(385, "Mouse disable", "Mouse pointer disable", 4));
		m_commands.push_back(AVSimpleCommand(386, "VOD", "Video on demand", 4));
		m_commands.push_back(AVSimpleCommand(387, "Thumbs Up", "Thumbs up for positive feedback in GUI", 4));
		m_commands.push_back(AVSimpleCommand(388, "Thumbs Down", "Thumbs down for negative feedback in GUI", 4));
		m_commands.push_back(AVSimpleCommand(389, "Apps", "Application selection/launch", 4));
		m_commands.push_back(AVSimpleCommand(390, "Mouse toggle", "Will toggle a mouse pointer between on and off", 4));
		m_commands.push_back(AVSimpleCommand(391, "TV Mode", "Will direct an AV device to go the TV mode (the mode is configured on the device)", 4));
		m_commands.push_back(AVSimpleCommand(392, "DVD Mode", "Will direct an AV device to go the DVD mode (the mode is configured on the device)", 4));
		m_commands.push_back(AVSimpleCommand(393, "STB Mode", "Will direct an AV device to go the STB mode (the mode is configured on the device)", 4));
		m_commands.push_back(AVSimpleCommand(394, "AUX Mode", "Will direct an AV device to go the AUX mode (the mode is configured on the device)", 4));
		m_commands.push_back(AVSimpleCommand(395, "BluRay Mode", "Will direct an AV device to go the BluRay mode (the mode is configured on the device)", 4));
		m_commands.push_back(AVSimpleCommand(404, "Standby 1", "Used for AV devices that support multiple standby mode. Power ON should be used to turn on the device", 4));
		m_commands.push_back(AVSimpleCommand(405, "Standby 2", "Used for AV devices that support multiple standby mode. Power ON should be used to turn on the device", 4));
		m_commands.push_back(AVSimpleCommand(406, "Standby 3", "Used for AV devices that support multiple standby mode. Power ON should be used to turn on the device", 4));
		m_commands.push_back(AVSimpleCommand(407, "HDMI 1", "Discrete command used to set an AV device to HDMI input 1", 4));
		m_commands.push_back(AVSimpleCommand(408, "HDMI 2", "Discrete command used to set an AV device to HDMI input 2", 4));
		m_commands.push_back(AVSimpleCommand(409, "HDMI 3", "Discrete command used to set an AV device to HDMI input 3", 4));
		m_commands.push_back(AVSimpleCommand(410, "HDMI 4", "Discrete command used to set an AV device to HDMI input 4", 4));
		m_commands.push_back(AVSimpleCommand(411, "HDMI 5", "Discrete command used to set an AV device to HDMI input 5", 4));
		m_commands.push_back(AVSimpleCommand(412, "HDMI 6", "Discrete command used to set an AV device to HDMI input 6", 4));
		m_commands.push_back(AVSimpleCommand(413, "HDMI 7", "Discrete command used to set an AV device to HDMI input 7", 4));
		m_commands.push_back(AVSimpleCommand(414, "HDMI 8", "Discrete command used to set an AV device to HDMI input 8", 4));
		m_commands.push_back(AVSimpleCommand(415, "HDMI 9", "Discrete command used to set an AV device to HDMI input 9", 4));
		m_commands.push_back(AVSimpleCommand(416, "USB 1", "Discrete command used to set an AV device to USB input 1", 4));
		m_commands.push_back(AVSimpleCommand(417, "USB 2", "Discrete command used to set an AV device to USB input 2", 4));
		m_commands.push_back(AVSimpleCommand(418, "USB 3", "Discrete command used to set an AV device to USB input 3", 4));
		m_commands.push_back(AVSimpleCommand(419, "USB 4", "Discrete command used to set an AV device to USB input 4", 4));
		m_commands.push_back(AVSimpleCommand(420, "USB 5", "Discrete command used to set an AV device to USB input 5", 4));
		m_commands.push_back(AVSimpleCommand(421, "ZOOM - 4:3 Normal", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(AVSimpleCommand(422, "ZOOM - 4:3 Zoom", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(AVSimpleCommand(423, "ZOOM - 16:9 Normal", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(AVSimpleCommand(424, "ZOOM - 16:9 Zoom", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(AVSimpleCommand(425, "ZOOM - 16:9 Wide 1", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(AVSimpleCommand(426, "ZOOM 16:9 Wide 2", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(AVSimpleCommand(427, "ZOOM 16:9 Wide 3", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(AVSimpleCommand(428, "ZOOM 16:9 Cinema", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(AVSimpleCommand(429, "ZOOM Default", "Discrete commands that is used to set a TV the default Zoom mode", 4));
		m_commands.push_back(AVSimpleCommand(432, "Auto Zoom", "Will set Zoom mode automatically", 4));
		m_commands.push_back(AVSimpleCommand(433, "ZOOM - Set as Default Zoom", "Will set the current active Zoom level to default", 4));
		m_commands.push_back(AVSimpleCommand(434, "Mute ON", "Discrete Mute ON command", 4));
		m_commands.push_back(AVSimpleCommand(435, "Mute OFF", "Discrete Mute OFF command", 4));
		m_commands.push_back(AVSimpleCommand(436, "AUDIO Mode - AUDYSSEY AUDIO OFF", "Discrete Audio mode for Audussey audio processing (Off) ", 4));
		m_commands.push_back(AVSimpleCommand(437, "AUDIO Mode - AUDYSSEY AUDIO LO", "Discrete Audio mode for Audussey audio processing (Low) ", 4));
		m_commands.push_back(AVSimpleCommand(438, "AUDIO Mode - AUDYSSEY AUDIO MED", "Discrete Audio mode for Audussey audio processing (Medium) ", 4));
		m_commands.push_back(AVSimpleCommand(439, "AUDIO Mode - AUDYSSEY AUDIO HI", "Discrete Audio mode for Audussey audio processing (High) ", 4));
		m_commands.push_back(AVSimpleCommand(442, "AUDIO Mode - SRS SURROUND ON", "Discrete Audio mode for SRS audio processing", 4));
		m_commands.push_back(AVSimpleCommand(443, "AUDIO Mode - SRS SURROUND OFF", "Discrete Audio mode for SRS audio processing", 4));
		m_commands.push_back(AVSimpleCommand(447, "Picture Mode - Home", "Discrete picture for TVs", 4));
		m_commands.push_back(AVSimpleCommand(448, "Picture Mode - Retail", "Discrete picture for TVs", 4));
		m_commands.push_back(AVSimpleCommand(449, "Picture Mode - Vivid", "Discrete picture for TVs", 4));
		m_commands.push_back(AVSimpleCommand(450, "Picture Mode - Standard", "Discrete picture for TVs", 4));
		m_commands.push_back(AVSimpleCommand(451, "Picture Mode - Theater", "Discrete picture for TVs", 4));
		m_commands.push_back(AVSimpleCommand(452, "Picture Mode - Sports", "Discrete picture for TVs", 4));
		m_commands.push_back(AVSimpleCommand(453, "Picture Mode - Energy savings ", "Discrete picture for TVs", 4));
		m_commands.push_back(AVSimpleCommand(454, "Picture Mode - Custom", "Discrete picture for TVs", 4));
		m_commands.push_back(AVSimpleCommand(455, "Cool", "Discrete picture temperature adjustments", 4));
		m_commands.push_back(AVSimpleCommand(456, "Medium", "Discrete picture temperature adjustments", 4));
		m_commands.push_back(AVSimpleCommand(457, "Warm_D65", "Discrete picture temperature adjustments", 4));
		m_commands.push_back(AVSimpleCommand(458, "CC ON", "Discrete Closed caption commands", 4));
		m_commands.push_back(AVSimpleCommand(459, "CC OFF", "Discrete Closed caption commands", 4));
		m_commands.push_back(AVSimpleCommand(460, "Video Mute ON", "Discrete Video mute command", 4));
		m_commands.push_back(AVSimpleCommand(461, "Video Mute OFF", "Discrete Video mute command", 4));
		m_commands.push_back(AVSimpleCommand(462, "Next Event", "Go to next state or event ", 4));
		m_commands.push_back(AVSimpleCommand(463, "Previous Event", "Go to previous state or event", 4));
		m_commands.push_back(AVSimpleCommand(464, "CEC device list", "Brings up the CES device list", 4));
		m_commands.push_back(AVSimpleCommand(465, "MTS SAP", "Secondary Audio programming", 4));
	}
	return m_commands;
}