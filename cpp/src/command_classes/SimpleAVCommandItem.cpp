#include "SimpleAVCommandItem.h"
#include <vector>
#include <string>
#include "Defs.h"

using namespace OpenZWave;

static vector<SimpleAVCommandItem> m_commands;

//-----------------------------------------------------------------------------
// <SimpleAVCommand::SimpleAVCommand>
// SimpleAVCommand constructor
//-----------------------------------------------------------------------------
SimpleAVCommandItem::SimpleAVCommandItem
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
// <SimpleAVCommand::GetCode>
// Get code of ZWave AV command
//-----------------------------------------------------------------------------
uint16 SimpleAVCommandItem::GetCode()
{
	return m_code;
}

//-----------------------------------------------------------------------------
// <SimpleAVCommand::GetVersion>
// Get version of AV simple command class of command
//-----------------------------------------------------------------------------
uint16 SimpleAVCommandItem::GetVersion()
{
	return m_version;
}

//-----------------------------------------------------------------------------
// <SimpleAVCommand::GetName>
// Get human-friendly name of AV command
//-----------------------------------------------------------------------------
string SimpleAVCommandItem::GetName()
{
	return m_name;
}

//-----------------------------------------------------------------------------
// <SimpleAVCommand::GetDescription>
// Get human-friendly description of AV command
//-----------------------------------------------------------------------------
string SimpleAVCommandItem::GetDescription()
{
	return m_description;
}

//-----------------------------------------------------------------------------
// <SimpleAVCommand::GetCommands>
// Get all available AV commands
//-----------------------------------------------------------------------------
vector<SimpleAVCommandItem> SimpleAVCommandItem::GetCommands()
{
	if (m_commands.size() == 0)
	{
		// Generated code
		m_commands.push_back(SimpleAVCommandItem(1, "Mute", "", 1));
		m_commands.push_back(SimpleAVCommandItem(2, "Volume Down", "Level Down", 1));
		m_commands.push_back(SimpleAVCommandItem(3, "Volume Up", "Level Up", 1));
		m_commands.push_back(SimpleAVCommandItem(4, "Channel Up", "Program Up", 1));
		m_commands.push_back(SimpleAVCommandItem(5, "Channel Down", "Program Down", 1));
		m_commands.push_back(SimpleAVCommandItem(6, "0", "Preset 10", 1));
		m_commands.push_back(SimpleAVCommandItem(7, "1", "Preset 1", 1));
		m_commands.push_back(SimpleAVCommandItem(8, "2", "Preset 2", 1));
		m_commands.push_back(SimpleAVCommandItem(9, "3", "Preset 3", 1));
		m_commands.push_back(SimpleAVCommandItem(10, "4", "Preset 4", 1));
		m_commands.push_back(SimpleAVCommandItem(11, "5", "Preset 5", 1));
		m_commands.push_back(SimpleAVCommandItem(12, "6", "Preset 6", 1));
		m_commands.push_back(SimpleAVCommandItem(13, "7", "Preset 7", 1));
		m_commands.push_back(SimpleAVCommandItem(14, "8", "Preset 8", 1));
		m_commands.push_back(SimpleAVCommandItem(15, "9", "Preset 9", 1));
		m_commands.push_back(SimpleAVCommandItem(16, "Last Channel", "Recall, Previous Channel (WMC)", 1));
		m_commands.push_back(SimpleAVCommandItem(17, "Display", "Info", 1));
		m_commands.push_back(SimpleAVCommandItem(18, "Favorite Channel", "Favorite", 1));
		m_commands.push_back(SimpleAVCommandItem(19, "Play", "", 1));
		m_commands.push_back(SimpleAVCommandItem(20, "Stop", "", 1));
		m_commands.push_back(SimpleAVCommandItem(21, "Pause", "Still", 1));
		m_commands.push_back(SimpleAVCommandItem(22, "Fast Forward", "Search Forward", 1));
		m_commands.push_back(SimpleAVCommandItem(23, "Rewind", "Search Reverse", 1));
		m_commands.push_back(SimpleAVCommandItem(24, "Instant Replay", "Replay", 1));
		m_commands.push_back(SimpleAVCommandItem(25, "Record", "", 1));
		m_commands.push_back(SimpleAVCommandItem(26, "AC3", "Dolby Digital", 1));
		m_commands.push_back(SimpleAVCommandItem(27, "PVR Menu", "Tivo", 1));
		m_commands.push_back(SimpleAVCommandItem(28, "Guide", "EPG", 1));
		m_commands.push_back(SimpleAVCommandItem(29, "Menu", "Settings", 1));
		m_commands.push_back(SimpleAVCommandItem(30, "Menu Up", "Adjust Up", 1));
		m_commands.push_back(SimpleAVCommandItem(31, "Menu Down", "Adjust Down", 1));
		m_commands.push_back(SimpleAVCommandItem(32, "Menu Left", "Cursor Left", 1));
		m_commands.push_back(SimpleAVCommandItem(33, "Menu Right", "Cursor Right", 1));
		m_commands.push_back(SimpleAVCommandItem(34, "Page Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(35, "Page Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(36, "Select", "OK", 1));
		m_commands.push_back(SimpleAVCommandItem(37, "Exit", "", 1));
		m_commands.push_back(SimpleAVCommandItem(38, "Input", "Input Select", 1));
		m_commands.push_back(SimpleAVCommandItem(39, "Power", "Standby", 1));
		m_commands.push_back(SimpleAVCommandItem(40, "Enter Channel", "Channel Enter", 1));
		m_commands.push_back(SimpleAVCommandItem(41, "10", "", 1));
		m_commands.push_back(SimpleAVCommandItem(42, "11", "", 1));
		m_commands.push_back(SimpleAVCommandItem(43, "12", "", 1));
		m_commands.push_back(SimpleAVCommandItem(44, "13", "", 1));
		m_commands.push_back(SimpleAVCommandItem(45, "14", "", 1));
		m_commands.push_back(SimpleAVCommandItem(46, "15", "", 1));
		m_commands.push_back(SimpleAVCommandItem(47, "16", "", 1));
		m_commands.push_back(SimpleAVCommandItem(48, "10", "10+", 1));
		m_commands.push_back(SimpleAVCommandItem(49, "20", "20+", 1));
		m_commands.push_back(SimpleAVCommandItem(50, "100", "", 1));
		m_commands.push_back(SimpleAVCommandItem(51, "-/--", "", 1));
		m_commands.push_back(SimpleAVCommandItem(52, "3-CH", "", 1));
		m_commands.push_back(SimpleAVCommandItem(53, "3D", "Simulated Stereo", 1));
		m_commands.push_back(SimpleAVCommandItem(54, "6-CH Input", "6 Channel", 1));
		m_commands.push_back(SimpleAVCommandItem(55, "A", "", 1));
		m_commands.push_back(SimpleAVCommandItem(56, "Add", "Write", 1));
		m_commands.push_back(SimpleAVCommandItem(57, "Alarm", "", 1));
		m_commands.push_back(SimpleAVCommandItem(58, "AM", "", 1));
		m_commands.push_back(SimpleAVCommandItem(59, "Analog", "", 1));
		m_commands.push_back(SimpleAVCommandItem(60, "Angle", "", 1));
		m_commands.push_back(SimpleAVCommandItem(61, "Antenna", "External", 1));
		m_commands.push_back(SimpleAVCommandItem(62, "Antenna East", "", 1));
		m_commands.push_back(SimpleAVCommandItem(63, "Antenna West", "", 1));
		m_commands.push_back(SimpleAVCommandItem(64, "Aspect", "Size", 1));
		m_commands.push_back(SimpleAVCommandItem(65, "Audio 1", "Audio", 1));
		m_commands.push_back(SimpleAVCommandItem(66, "Audio 2", "", 1));
		m_commands.push_back(SimpleAVCommandItem(67, "Audio 3", "", 1));
		m_commands.push_back(SimpleAVCommandItem(68, "Audio Dubbing", "", 1));
		m_commands.push_back(SimpleAVCommandItem(69, "Audio Level Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(70, "Audio Level Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(71, "Auto/Manual", "", 1));
		m_commands.push_back(SimpleAVCommandItem(72, "Aux 1", "Aux", 1));
		m_commands.push_back(SimpleAVCommandItem(73, "Aux 2", "", 1));
		m_commands.push_back(SimpleAVCommandItem(74, "B", "", 1));
		m_commands.push_back(SimpleAVCommandItem(75, "Back", "Previous Screen", 1));
		m_commands.push_back(SimpleAVCommandItem(76, "Background", "Backlight", 1));
		m_commands.push_back(SimpleAVCommandItem(77, "Balance", "", 1));
		m_commands.push_back(SimpleAVCommandItem(78, "Balance Left", "", 1));
		m_commands.push_back(SimpleAVCommandItem(79, "Balance Right", "", 1));
		m_commands.push_back(SimpleAVCommandItem(80, "Band", "FM/AM", 1));
		m_commands.push_back(SimpleAVCommandItem(81, "Bandwidth", "Wide/Narrow", 1));
		m_commands.push_back(SimpleAVCommandItem(82, "Bass", "", 1));
		m_commands.push_back(SimpleAVCommandItem(83, "Bass Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(84, "Bass Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(85, "Blank", "", 1));
		m_commands.push_back(SimpleAVCommandItem(86, "Breeze Mode", "", 1));
		m_commands.push_back(SimpleAVCommandItem(87, "Bright", "Brighten", 1));
		m_commands.push_back(SimpleAVCommandItem(88, "Brightness", "", 1));
		m_commands.push_back(SimpleAVCommandItem(89, "Brightness Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(90, "Brightness Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(91, "Buy", "", 1));
		m_commands.push_back(SimpleAVCommandItem(92, "C", "", 1));
		m_commands.push_back(SimpleAVCommandItem(93, "Camera", "", 1));
		m_commands.push_back(SimpleAVCommandItem(94, "Category Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(95, "Category Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(96, "Center", "", 1));
		m_commands.push_back(SimpleAVCommandItem(97, "Center Down", "Center Volume Down", 1));
		m_commands.push_back(SimpleAVCommandItem(98, "Center Mode", "", 1));
		m_commands.push_back(SimpleAVCommandItem(99, "Center Up", "Center Volume Up", 1));
		m_commands.push_back(SimpleAVCommandItem(100, "Channel/Program", "C/P", 1));
		m_commands.push_back(SimpleAVCommandItem(101, "Clear", "Cancel", 1));
		m_commands.push_back(SimpleAVCommandItem(102, "Close", "", 1));
		m_commands.push_back(SimpleAVCommandItem(103, "Closed Caption", "CC", 1));
		m_commands.push_back(SimpleAVCommandItem(104, "Cold", "A/C", 1));
		m_commands.push_back(SimpleAVCommandItem(105, "Color", "", 1));
		m_commands.push_back(SimpleAVCommandItem(106, "Color Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(107, "Color Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(108, "Component 1", "RGB 1", 1));
		m_commands.push_back(SimpleAVCommandItem(109, "Component 2", "RGB 2", 1));
		m_commands.push_back(SimpleAVCommandItem(110, "Component 3", "", 1));
		m_commands.push_back(SimpleAVCommandItem(111, "Concert", "", 1));
		m_commands.push_back(SimpleAVCommandItem(112, "Confirm", "Check", 1));
		m_commands.push_back(SimpleAVCommandItem(113, "Continue", "Continuous", 1));
		m_commands.push_back(SimpleAVCommandItem(114, "Contrast", "", 1));
		m_commands.push_back(SimpleAVCommandItem(115, "Contrast Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(116, "Contrast Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(117, "Counter", "", 1));
		m_commands.push_back(SimpleAVCommandItem(118, "Counter Reset", "", 1));
		m_commands.push_back(SimpleAVCommandItem(119, "D", "", 1));
		m_commands.push_back(SimpleAVCommandItem(120, "Day Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(121, "Day Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(122, "Delay", "", 1));
		m_commands.push_back(SimpleAVCommandItem(123, "Delay Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(124, "Delay Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(125, "Delete", "Erase", 1));
		m_commands.push_back(SimpleAVCommandItem(126, "Delimiter", "Sub-Channel", 1));
		m_commands.push_back(SimpleAVCommandItem(127, "Digest", "", 1));
		m_commands.push_back(SimpleAVCommandItem(128, "Digital", "", 1));
		m_commands.push_back(SimpleAVCommandItem(129, "Dim", "Dimmer", 1));
		m_commands.push_back(SimpleAVCommandItem(130, "Direct", "", 1));
		m_commands.push_back(SimpleAVCommandItem(131, "Disarm", "", 1));
		m_commands.push_back(SimpleAVCommandItem(132, "Disc", "", 1));
		m_commands.push_back(SimpleAVCommandItem(133, "Disc 1", "", 1));
		m_commands.push_back(SimpleAVCommandItem(134, "Disc 2", "", 1));
		m_commands.push_back(SimpleAVCommandItem(135, "Disc 3", "", 1));
		m_commands.push_back(SimpleAVCommandItem(136, "Disc 4", "", 1));
		m_commands.push_back(SimpleAVCommandItem(137, "Disc 5", "", 1));
		m_commands.push_back(SimpleAVCommandItem(138, "Disc 6", "", 1));
		m_commands.push_back(SimpleAVCommandItem(139, "Disc Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(140, "Disc Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(141, "Disco", "", 1));
		m_commands.push_back(SimpleAVCommandItem(142, "Edit", "", 1));
		m_commands.push_back(SimpleAVCommandItem(143, "Effect Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(144, "Effect Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(145, "Eject", "Open/Close", 1));
		m_commands.push_back(SimpleAVCommandItem(146, "End", "", 1));
		m_commands.push_back(SimpleAVCommandItem(147, "EQ", "Equalizer", 1));
		m_commands.push_back(SimpleAVCommandItem(148, "Fader", "", 1));
		m_commands.push_back(SimpleAVCommandItem(149, "Fan", "", 1));
		m_commands.push_back(SimpleAVCommandItem(150, "Fan High", "", 1));
		m_commands.push_back(SimpleAVCommandItem(151, "Fan Low", "", 1));
		m_commands.push_back(SimpleAVCommandItem(152, "Fan Medium", "", 1));
		m_commands.push_back(SimpleAVCommandItem(153, "Fan Speed", "", 1));
		m_commands.push_back(SimpleAVCommandItem(154, "Fastext Blue", "", 1));
		m_commands.push_back(SimpleAVCommandItem(155, "Fastext Green", "", 1));
		m_commands.push_back(SimpleAVCommandItem(156, "Fastext Purple", "", 1));
		m_commands.push_back(SimpleAVCommandItem(157, "Fastext Red", "", 1));
		m_commands.push_back(SimpleAVCommandItem(158, "Fastext White", "", 1));
		m_commands.push_back(SimpleAVCommandItem(159, "Fastext Yellow", "", 1));
		m_commands.push_back(SimpleAVCommandItem(160, "Favorite Channel Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(161, "Favorite Channel Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(162, "Finalize", "", 1));
		m_commands.push_back(SimpleAVCommandItem(163, "Fine Tune", "", 1));
		m_commands.push_back(SimpleAVCommandItem(164, "Flat", "", 1));
		m_commands.push_back(SimpleAVCommandItem(165, "FM", "", 1));
		m_commands.push_back(SimpleAVCommandItem(166, "Focus Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(167, "Focus Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(168, "Freeze", "", 1));
		m_commands.push_back(SimpleAVCommandItem(169, "Front", "", 1));
		m_commands.push_back(SimpleAVCommandItem(170, "Game", "", 1));
		m_commands.push_back(SimpleAVCommandItem(171, "GoTo", "Index Search", 1));
		m_commands.push_back(SimpleAVCommandItem(172, "Hall", "", 1));
		m_commands.push_back(SimpleAVCommandItem(173, "Heat", "", 1));
		m_commands.push_back(SimpleAVCommandItem(174, "Help", "", 1));
		m_commands.push_back(SimpleAVCommandItem(175, "Home", "", 1));
		m_commands.push_back(SimpleAVCommandItem(176, "Index", "VISS", 1));
		m_commands.push_back(SimpleAVCommandItem(177, "Index Forward", "", 1));
		m_commands.push_back(SimpleAVCommandItem(178, "Index Reverse", "", 1));
		m_commands.push_back(SimpleAVCommandItem(179, "Interactive", "Planner", 1));
		m_commands.push_back(SimpleAVCommandItem(180, "Intro Scan", "", 1));
		m_commands.push_back(SimpleAVCommandItem(181, "Jazz", "", 1));
		m_commands.push_back(SimpleAVCommandItem(182, "Karaoke", "", 1));
		m_commands.push_back(SimpleAVCommandItem(183, "Keystone", "", 1));
		m_commands.push_back(SimpleAVCommandItem(184, "Keystone Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(185, "Keystone Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(186, "Language", "SAP", 1));
		m_commands.push_back(SimpleAVCommandItem(187, "Left Click", "", 1));
		m_commands.push_back(SimpleAVCommandItem(188, "Level", "Volume", 1));
		m_commands.push_back(SimpleAVCommandItem(189, "Light", "Lamp", 1));
		m_commands.push_back(SimpleAVCommandItem(190, "List", "My Shows", 1));
		m_commands.push_back(SimpleAVCommandItem(191, "Live TV", "Return to Live", 1));
		m_commands.push_back(SimpleAVCommandItem(192, "Local/Dx", "", 1));
		m_commands.push_back(SimpleAVCommandItem(193, "Loudness", "", 1));
		m_commands.push_back(SimpleAVCommandItem(194, "Mail", "Email", 1));
		m_commands.push_back(SimpleAVCommandItem(195, "Mark", "Bookmark", 1));
		m_commands.push_back(SimpleAVCommandItem(196, "Memory Recall", "", 1));
		m_commands.push_back(SimpleAVCommandItem(197, "Monitor", "Tape Monitor", 1));
		m_commands.push_back(SimpleAVCommandItem(198, "Movie", "", 1));
		m_commands.push_back(SimpleAVCommandItem(199, "Multi Room", "", 1));
		m_commands.push_back(SimpleAVCommandItem(200, "Music", "TV/Radio, My Music (WMC)", 1));
		m_commands.push_back(SimpleAVCommandItem(201, "Music Scan", "Memory Scan", 1));
		m_commands.push_back(SimpleAVCommandItem(202, "Natural", "", 1));
		m_commands.push_back(SimpleAVCommandItem(203, "Night", "", 1));
		m_commands.push_back(SimpleAVCommandItem(204, "Noise Reduction", "Dolby NR", 1));
		m_commands.push_back(SimpleAVCommandItem(205, "Normalize", "Personal Preference", 1));
		m_commands.push_back(SimpleAVCommandItem(206, "Discrete input Cable", "CATV", 1));
		m_commands.push_back(SimpleAVCommandItem(207, "Discrete input CD 1", "CD", 1));
		m_commands.push_back(SimpleAVCommandItem(208, "Discrete input CD 2", "CDR", 1));
		m_commands.push_back(SimpleAVCommandItem(209, "Discrete input CDR", "Compact Disc Recorder", 1));
		m_commands.push_back(SimpleAVCommandItem(210, "Discrete input DAT", "Digital Audio Tape", 1));
		m_commands.push_back(SimpleAVCommandItem(211, "Discrete input DVD", "Digital Video Disk", 1));
		m_commands.push_back(SimpleAVCommandItem(212, "Discrete input DVI", "Digital Video Interface", 1));
		m_commands.push_back(SimpleAVCommandItem(213, "Discrete input HDTV", "", 1));
		m_commands.push_back(SimpleAVCommandItem(214, "Discrete input LD", "Laser Disc", 1));
		m_commands.push_back(SimpleAVCommandItem(215, "Discrete input MD", "Mini Disc", 1));
		m_commands.push_back(SimpleAVCommandItem(216, "Discrete input PC", "Personal Computer", 1));
		m_commands.push_back(SimpleAVCommandItem(217, "Discrete input PVR", "Personal Video Recorder", 1));
		m_commands.push_back(SimpleAVCommandItem(218, "Discrete input TV", "", 1));
		m_commands.push_back(SimpleAVCommandItem(219, "Discrete input TV/VCR", "TV/DVD", 1));
		m_commands.push_back(SimpleAVCommandItem(220, "Discrete input VCR", "", 1));
		m_commands.push_back(SimpleAVCommandItem(221, "One Touch Playback", "OTPB", 1));
		m_commands.push_back(SimpleAVCommandItem(222, "One Touch Record", "OTR", 1));
		m_commands.push_back(SimpleAVCommandItem(223, "Open", "", 1));
		m_commands.push_back(SimpleAVCommandItem(224, "Optical", "", 1));
		m_commands.push_back(SimpleAVCommandItem(225, "Options", "", 1));
		m_commands.push_back(SimpleAVCommandItem(226, "Orchestra", "", 1));
		m_commands.push_back(SimpleAVCommandItem(227, "PAL/NTSC", "System Select", 1));
		m_commands.push_back(SimpleAVCommandItem(228, "Parental Lock", "Parental Control", 1));
		m_commands.push_back(SimpleAVCommandItem(229, "PBC", "Playback Control", 1));
		m_commands.push_back(SimpleAVCommandItem(230, "Phono", "", 1));
		m_commands.push_back(SimpleAVCommandItem(231, "Photos", "Pictures, My Pictures (WMC)", 1));
		m_commands.push_back(SimpleAVCommandItem(232, "Picture Menu", "Picture Adjust", 1));
		m_commands.push_back(SimpleAVCommandItem(233, "Picture Mode", "Smart Picture", 1));
		m_commands.push_back(SimpleAVCommandItem(234, "Picture Mute", "", 1));
		m_commands.push_back(SimpleAVCommandItem(235, "PIP Channel Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(236, "PIP Channel Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(237, "PIP Freeze", "", 1));
		m_commands.push_back(SimpleAVCommandItem(238, "PIP Input", "PIP Mode", 1));
		m_commands.push_back(SimpleAVCommandItem(239, "PIP Move", "PIP Position", 1));
		m_commands.push_back(SimpleAVCommandItem(240, "PIP Off", "", 1));
		m_commands.push_back(SimpleAVCommandItem(241, "PIP On", "PIP", 1));
		m_commands.push_back(SimpleAVCommandItem(242, "PIP Size", "", 1));
		m_commands.push_back(SimpleAVCommandItem(243, "PIP Split", "Multi Screen", 1));
		m_commands.push_back(SimpleAVCommandItem(244, "PIP Swap", "PIP Exchange", 1));
		m_commands.push_back(SimpleAVCommandItem(245, "Play Mode", "", 1));
		m_commands.push_back(SimpleAVCommandItem(246, "Play Reverse", "", 1));
		m_commands.push_back(SimpleAVCommandItem(247, "Power Off", "", 1));
		m_commands.push_back(SimpleAVCommandItem(248, "Power On", "", 1));
		m_commands.push_back(SimpleAVCommandItem(249, "PPV", "Pay Per View", 1));
		m_commands.push_back(SimpleAVCommandItem(250, "Preset", "", 1));
		m_commands.push_back(SimpleAVCommandItem(251, "Program", "Program Memory", 1));
		m_commands.push_back(SimpleAVCommandItem(252, "Progressive Scan", "Progressive", 1));
		m_commands.push_back(SimpleAVCommandItem(253, "ProLogic", "Dolby Prologic", 1));
		m_commands.push_back(SimpleAVCommandItem(254, "PTY", "Audio Program Type", 1));
		m_commands.push_back(SimpleAVCommandItem(255, "Quick Skip", "Commercial Skip", 1));
		m_commands.push_back(SimpleAVCommandItem(256, "Random", "Shuffle", 1));
		m_commands.push_back(SimpleAVCommandItem(257, "RDS", "Radio Data System", 1));
		m_commands.push_back(SimpleAVCommandItem(258, "Rear", "", 1));
		m_commands.push_back(SimpleAVCommandItem(259, "Rear Volume Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(260, "Rear Volume Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(261, "Record Mute", "", 1));
		m_commands.push_back(SimpleAVCommandItem(262, "Record Pause", "", 1));
		m_commands.push_back(SimpleAVCommandItem(263, "Repeat", "", 1));
		m_commands.push_back(SimpleAVCommandItem(264, "Repeat A-B", "", 1));
		m_commands.push_back(SimpleAVCommandItem(265, "Resume", "", 1));
		m_commands.push_back(SimpleAVCommandItem(266, "RGB", "Red Green Blue Component Video", 1));
		m_commands.push_back(SimpleAVCommandItem(267, "Right Click", "", 1));
		m_commands.push_back(SimpleAVCommandItem(268, "Rock", "", 1));
		m_commands.push_back(SimpleAVCommandItem(269, "Rotate Left", "", 1));
		m_commands.push_back(SimpleAVCommandItem(270, "Rotate Right", "", 1));
		m_commands.push_back(SimpleAVCommandItem(271, "SAT", "Sky", 1));
		m_commands.push_back(SimpleAVCommandItem(272, "Scan", "Channel Scan", 1));
		m_commands.push_back(SimpleAVCommandItem(273, "Scart", "", 1));
		m_commands.push_back(SimpleAVCommandItem(274, "Scene", "", 1));
		m_commands.push_back(SimpleAVCommandItem(275, "Scroll", "", 1));
		m_commands.push_back(SimpleAVCommandItem(276, "Services", "", 1));
		m_commands.push_back(SimpleAVCommandItem(277, "Setup Menu", "Setup", 1));
		m_commands.push_back(SimpleAVCommandItem(278, "Sharp", "", 1));
		m_commands.push_back(SimpleAVCommandItem(279, "Sharpness", "", 1));
		m_commands.push_back(SimpleAVCommandItem(280, "Sharpness Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(281, "Sharpness Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(282, "Side A/B", "", 1));
		m_commands.push_back(SimpleAVCommandItem(283, "Skip Forward", "Next", 1));
		m_commands.push_back(SimpleAVCommandItem(284, "Skip Reverse", "Previous", 1));
		m_commands.push_back(SimpleAVCommandItem(285, "Sleep", "Off Timer", 1));
		m_commands.push_back(SimpleAVCommandItem(286, "Slow", "", 1));
		m_commands.push_back(SimpleAVCommandItem(287, "Slow Forward", "", 1));
		m_commands.push_back(SimpleAVCommandItem(288, "Slow Reverse", "", 1));
		m_commands.push_back(SimpleAVCommandItem(289, "Sound Menu", "Audio Menu", 1));
		m_commands.push_back(SimpleAVCommandItem(290, "Sound Mode", "Smart Sound", 1));
		m_commands.push_back(SimpleAVCommandItem(291, "Speed", "Record Speed", 1));
		m_commands.push_back(SimpleAVCommandItem(292, "Speed Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(293, "Speed Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(294, "Sports", "Digital Surround Processing", 1));
		m_commands.push_back(SimpleAVCommandItem(295, "Stadium", "", 1));
		m_commands.push_back(SimpleAVCommandItem(296, "Start", "", 1));
		m_commands.push_back(SimpleAVCommandItem(297, "Start ID Erase", "Erase", 1));
		m_commands.push_back(SimpleAVCommandItem(298, "Start ID Renumber", "Renumber", 1));
		m_commands.push_back(SimpleAVCommandItem(299, "Start ID Write", "Write", 1));
		m_commands.push_back(SimpleAVCommandItem(300, "Step", "", 1));
		m_commands.push_back(SimpleAVCommandItem(301, "Stereo/Mono", "L/R", 1));
		m_commands.push_back(SimpleAVCommandItem(302, "Still Forward", "Frame Advance", 1));
		m_commands.push_back(SimpleAVCommandItem(303, "Still Reverse", "Frame Reverse", 1));
		m_commands.push_back(SimpleAVCommandItem(304, "Subtitle", "Subtitle On-Off", 1));
		m_commands.push_back(SimpleAVCommandItem(305, "Subwoofer Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(306, "Subwoofer Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(307, "Super Bass", "Bass Boost", 1));
		m_commands.push_back(SimpleAVCommandItem(308, "Surround", "", 1));
		m_commands.push_back(SimpleAVCommandItem(309, "Surround Mode", "Sound Field", 1));
		m_commands.push_back(SimpleAVCommandItem(310, "S-Video", "", 1));
		m_commands.push_back(SimpleAVCommandItem(311, "Sweep", "Oscillate", 1));
		m_commands.push_back(SimpleAVCommandItem(312, "Synchro Record", "CD Synchro", 1));
		m_commands.push_back(SimpleAVCommandItem(313, "Tape 1", "Deck 1", 1));
		m_commands.push_back(SimpleAVCommandItem(314, "Tape 1-2", "Deck 1-2", 1));
		m_commands.push_back(SimpleAVCommandItem(315, "Tape 2", "Deck 2", 1));
		m_commands.push_back(SimpleAVCommandItem(316, "Temperature Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(317, "Temperature Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(318, "Test Tone", "", 1));
		m_commands.push_back(SimpleAVCommandItem(319, "Text", "Teletext", 1));
		m_commands.push_back(SimpleAVCommandItem(320, "Text Expand", "", 1));
		m_commands.push_back(SimpleAVCommandItem(321, "Text Hold", "", 1));
		m_commands.push_back(SimpleAVCommandItem(322, "Text Index", "", 1));
		m_commands.push_back(SimpleAVCommandItem(323, "Text Mix", "", 1));
		m_commands.push_back(SimpleAVCommandItem(324, "Text Off", "", 1));
		m_commands.push_back(SimpleAVCommandItem(325, "Text Reveal", "", 1));
		m_commands.push_back(SimpleAVCommandItem(326, "Text Subpage", "", 1));
		m_commands.push_back(SimpleAVCommandItem(327, "Text Timed Page", "", 1));
		m_commands.push_back(SimpleAVCommandItem(328, "Text Update", "Text Cancel", 1));
		m_commands.push_back(SimpleAVCommandItem(329, "Theater", "Cinema EQ", 1));
		m_commands.push_back(SimpleAVCommandItem(330, "Theme", "Category Select", 1));
		m_commands.push_back(SimpleAVCommandItem(331, "Thumbs Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(332, "Thumbs Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(333, "Tilt Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(334, "Tilt Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(335, "Time", "Clock", 1));
		m_commands.push_back(SimpleAVCommandItem(336, "Timer", "", 1));
		m_commands.push_back(SimpleAVCommandItem(337, "Timer Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(338, "Timer Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(339, "Tint", "", 1));
		m_commands.push_back(SimpleAVCommandItem(340, "Tint Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(341, "Tint Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(342, "Title", "Top Menu", 1));
		m_commands.push_back(SimpleAVCommandItem(343, "Track", "Chapter", 1));
		m_commands.push_back(SimpleAVCommandItem(344, "Tracking", "", 1));
		m_commands.push_back(SimpleAVCommandItem(345, "Tracking Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(346, "Tracking Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(347, "Treble", "", 1));
		m_commands.push_back(SimpleAVCommandItem(348, "Treble Down", "", 1));
		m_commands.push_back(SimpleAVCommandItem(349, "Treble Up", "", 1));
		m_commands.push_back(SimpleAVCommandItem(350, "Tune Down", "Audio Tune Down", 1));
		m_commands.push_back(SimpleAVCommandItem(351, "Tune Up", "Audio Tune Up", 1));
		m_commands.push_back(SimpleAVCommandItem(352, "Tuner", "", 1));
		m_commands.push_back(SimpleAVCommandItem(353, "VCR Plus+", "Showview", 1));
		m_commands.push_back(SimpleAVCommandItem(354, "Video 1", "A/V 1", 1));
		m_commands.push_back(SimpleAVCommandItem(355, "Video 2", "A/V 2", 1));
		m_commands.push_back(SimpleAVCommandItem(356, "Video 3", "A/V 3", 1));
		m_commands.push_back(SimpleAVCommandItem(357, "Video 4", "A/V 4", 1));
		m_commands.push_back(SimpleAVCommandItem(358, "Video 5", "A/V 5", 1));
		m_commands.push_back(SimpleAVCommandItem(359, "View", "", 1));
		m_commands.push_back(SimpleAVCommandItem(360, "Voice", "Vocals", 1));
		m_commands.push_back(SimpleAVCommandItem(361, "Zoom", "Magnify", 1));
		m_commands.push_back(SimpleAVCommandItem(362, "Zoom In", "Zoom Up", 1));
		m_commands.push_back(SimpleAVCommandItem(363, "Zoom Out", "Zoom Down", 1));
		m_commands.push_back(SimpleAVCommandItem(364, "eHome", "(Windows Media Center)", 2));
		m_commands.push_back(SimpleAVCommandItem(365, "Details", "(Windows Media Center)", 2));
		m_commands.push_back(SimpleAVCommandItem(366, "DVD Menu", "(Windows Media Center)", 2));
		m_commands.push_back(SimpleAVCommandItem(367, "My TV", "(Windows Media Center)", 2));
		m_commands.push_back(SimpleAVCommandItem(368, "Recorded TV", "(Windows Media Center)", 2));
		m_commands.push_back(SimpleAVCommandItem(369, "My Videos", "(Windows Media Center)", 2));
		m_commands.push_back(SimpleAVCommandItem(370, "DVD Angle", "(Windows Media Center)", 2));
		m_commands.push_back(SimpleAVCommandItem(371, "DVD Audio", "(Windows Media Center)", 2));
		m_commands.push_back(SimpleAVCommandItem(372, "DVD Subtitle", "(Windows Media Center)", 2));
		m_commands.push_back(SimpleAVCommandItem(373, "Radio", "(Windows Media Center)", 2));
		m_commands.push_back(SimpleAVCommandItem(374, "#", "(Windows Media Center)", 2));
		m_commands.push_back(SimpleAVCommandItem(375, "*", "(Windows Media Center)", 2));
		m_commands.push_back(SimpleAVCommandItem(376, "OEM 1", "(Windows Media Center)", 2));
		m_commands.push_back(SimpleAVCommandItem(377, "OEM 2", "(Windows Media Center)", 2));
		m_commands.push_back(SimpleAVCommandItem(378, "Info", "Used to request information", 3));
		m_commands.push_back(SimpleAVCommandItem(379, "CAPS NUM", "Switch between numeric and alpha (Shift)", 3));
		m_commands.push_back(SimpleAVCommandItem(380, "TV MODE", "Cycles through video output modes/resolutions", 3));
		m_commands.push_back(SimpleAVCommandItem(381, "SOURCE", "Displays the possible sources for the playback. [NFS, ext., USB, UPnP]", 3));
		m_commands.push_back(SimpleAVCommandItem(382, "FILEMODE", "File manipulation. Add/remove to list, create folder, rename file", 3));
		m_commands.push_back(SimpleAVCommandItem(383, "Time Seek", "This seeks to time position. Used for DVD/CD/others", 3));
		m_commands.push_back(SimpleAVCommandItem(384, "Mouse enable", "Mouse pointer enable", 4));
		m_commands.push_back(SimpleAVCommandItem(385, "Mouse disable", "Mouse pointer disable", 4));
		m_commands.push_back(SimpleAVCommandItem(386, "VOD", "Video on demand", 4));
		m_commands.push_back(SimpleAVCommandItem(387, "Thumbs Up", "Thumbs up for positive feedback in GUI", 4));
		m_commands.push_back(SimpleAVCommandItem(388, "Thumbs Down", "Thumbs down for negative feedback in GUI", 4));
		m_commands.push_back(SimpleAVCommandItem(389, "Apps", "Application selection/launch", 4));
		m_commands.push_back(SimpleAVCommandItem(390, "Mouse toggle", "Will toggle a mouse pointer between on and off", 4));
		m_commands.push_back(SimpleAVCommandItem(391, "TV Mode", "Will direct an AV device to go the TV mode (the mode is configured on the device)", 4));
		m_commands.push_back(SimpleAVCommandItem(392, "DVD Mode", "Will direct an AV device to go the DVD mode (the mode is configured on the device)", 4));
		m_commands.push_back(SimpleAVCommandItem(393, "STB Mode", "Will direct an AV device to go the STB mode (the mode is configured on the device)", 4));
		m_commands.push_back(SimpleAVCommandItem(394, "AUX Mode", "Will direct an AV device to go the AUX mode (the mode is configured on the device)", 4));
		m_commands.push_back(SimpleAVCommandItem(395, "BluRay Mode", "Will direct an AV device to go the BluRay mode (the mode is configured on the device)", 4));
		m_commands.push_back(SimpleAVCommandItem(404, "Standby 1", "Used for AV devices that support multiple standby mode. Power ON should be used to turn on the device", 4));
		m_commands.push_back(SimpleAVCommandItem(405, "Standby 2", "Used for AV devices that support multiple standby mode. Power ON should be used to turn on the device", 4));
		m_commands.push_back(SimpleAVCommandItem(406, "Standby 3", "Used for AV devices that support multiple standby mode. Power ON should be used to turn on the device", 4));
		m_commands.push_back(SimpleAVCommandItem(407, "HDMI 1", "Discrete command used to set an AV device to HDMI input 1", 4));
		m_commands.push_back(SimpleAVCommandItem(408, "HDMI 2", "Discrete command used to set an AV device to HDMI input 2", 4));
		m_commands.push_back(SimpleAVCommandItem(409, "HDMI 3", "Discrete command used to set an AV device to HDMI input 3", 4));
		m_commands.push_back(SimpleAVCommandItem(410, "HDMI 4", "Discrete command used to set an AV device to HDMI input 4", 4));
		m_commands.push_back(SimpleAVCommandItem(411, "HDMI 5", "Discrete command used to set an AV device to HDMI input 5", 4));
		m_commands.push_back(SimpleAVCommandItem(412, "HDMI 6", "Discrete command used to set an AV device to HDMI input 6", 4));
		m_commands.push_back(SimpleAVCommandItem(413, "HDMI 7", "Discrete command used to set an AV device to HDMI input 7", 4));
		m_commands.push_back(SimpleAVCommandItem(414, "HDMI 8", "Discrete command used to set an AV device to HDMI input 8", 4));
		m_commands.push_back(SimpleAVCommandItem(415, "HDMI 9", "Discrete command used to set an AV device to HDMI input 9", 4));
		m_commands.push_back(SimpleAVCommandItem(416, "USB 1", "Discrete command used to set an AV device to USB input 1", 4));
		m_commands.push_back(SimpleAVCommandItem(417, "USB 2", "Discrete command used to set an AV device to USB input 2", 4));
		m_commands.push_back(SimpleAVCommandItem(418, "USB 3", "Discrete command used to set an AV device to USB input 3", 4));
		m_commands.push_back(SimpleAVCommandItem(419, "USB 4", "Discrete command used to set an AV device to USB input 4", 4));
		m_commands.push_back(SimpleAVCommandItem(420, "USB 5", "Discrete command used to set an AV device to USB input 5", 4));
		m_commands.push_back(SimpleAVCommandItem(421, "ZOOM - 4:3 Normal", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(SimpleAVCommandItem(422, "ZOOM - 4:3 Zoom", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(SimpleAVCommandItem(423, "ZOOM - 16:9 Normal", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(SimpleAVCommandItem(424, "ZOOM - 16:9 Zoom", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(SimpleAVCommandItem(425, "ZOOM - 16:9 Wide 1", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(SimpleAVCommandItem(426, "ZOOM 16:9 Wide 2", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(SimpleAVCommandItem(427, "ZOOM 16:9 Wide 3", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(SimpleAVCommandItem(428, "ZOOM 16:9 Cinema", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(SimpleAVCommandItem(429, "ZOOM Default", "Discrete commands that is used to set a TV the default Zoom mode", 4));
		m_commands.push_back(SimpleAVCommandItem(432, "Auto Zoom", "Will set Zoom mode automatically", 4));
		m_commands.push_back(SimpleAVCommandItem(433, "ZOOM - Set as Default Zoom", "Will set the current active Zoom level to default", 4));
		m_commands.push_back(SimpleAVCommandItem(434, "Mute ON", "Discrete Mute ON command", 4));
		m_commands.push_back(SimpleAVCommandItem(435, "Mute OFF", "Discrete Mute OFF command", 4));
		m_commands.push_back(SimpleAVCommandItem(436, "AUDIO Mode - AUDYSSEY AUDIO OFF", "Discrete Audio mode for Audussey audio processing (Off) ", 4));
		m_commands.push_back(SimpleAVCommandItem(437, "AUDIO Mode - AUDYSSEY AUDIO LO", "Discrete Audio mode for Audussey audio processing (Low) ", 4));
		m_commands.push_back(SimpleAVCommandItem(438, "AUDIO Mode - AUDYSSEY AUDIO MED", "Discrete Audio mode for Audussey audio processing (Medium) ", 4));
		m_commands.push_back(SimpleAVCommandItem(439, "AUDIO Mode - AUDYSSEY AUDIO HI", "Discrete Audio mode for Audussey audio processing (High) ", 4));
		m_commands.push_back(SimpleAVCommandItem(442, "AUDIO Mode - SRS SURROUND ON", "Discrete Audio mode for SRS audio processing", 4));
		m_commands.push_back(SimpleAVCommandItem(443, "AUDIO Mode - SRS SURROUND OFF", "Discrete Audio mode for SRS audio processing", 4));
		m_commands.push_back(SimpleAVCommandItem(447, "Picture Mode - Home", "Discrete picture for TVs", 4));
		m_commands.push_back(SimpleAVCommandItem(448, "Picture Mode - Retail", "Discrete picture for TVs", 4));
		m_commands.push_back(SimpleAVCommandItem(449, "Picture Mode - Vivid", "Discrete picture for TVs", 4));
		m_commands.push_back(SimpleAVCommandItem(450, "Picture Mode - Standard", "Discrete picture for TVs", 4));
		m_commands.push_back(SimpleAVCommandItem(451, "Picture Mode - Theater", "Discrete picture for TVs", 4));
		m_commands.push_back(SimpleAVCommandItem(452, "Picture Mode - Sports", "Discrete picture for TVs", 4));
		m_commands.push_back(SimpleAVCommandItem(453, "Picture Mode - Energy savings ", "Discrete picture for TVs", 4));
		m_commands.push_back(SimpleAVCommandItem(454, "Picture Mode - Custom", "Discrete picture for TVs", 4));
		m_commands.push_back(SimpleAVCommandItem(455, "Cool", "Discrete picture temperature adjustments", 4));
		m_commands.push_back(SimpleAVCommandItem(456, "Medium", "Discrete picture temperature adjustments", 4));
		m_commands.push_back(SimpleAVCommandItem(457, "Warm_D65", "Discrete picture temperature adjustments", 4));
		m_commands.push_back(SimpleAVCommandItem(458, "CC ON", "Discrete Closed caption commands", 4));
		m_commands.push_back(SimpleAVCommandItem(459, "CC OFF", "Discrete Closed caption commands", 4));
		m_commands.push_back(SimpleAVCommandItem(460, "Video Mute ON", "Discrete Video mute command", 4));
		m_commands.push_back(SimpleAVCommandItem(461, "Video Mute OFF", "Discrete Video mute command", 4));
		m_commands.push_back(SimpleAVCommandItem(462, "Next Event", "Go to next state or event ", 4));
		m_commands.push_back(SimpleAVCommandItem(463, "Previous Event", "Go to previous state or event", 4));
		m_commands.push_back(SimpleAVCommandItem(464, "CEC device list", "Brings up the CES device list", 4));
		m_commands.push_back(SimpleAVCommandItem(465, "MTS SAP", "Secondary Audio programming", 4));
	}
	return m_commands;
}