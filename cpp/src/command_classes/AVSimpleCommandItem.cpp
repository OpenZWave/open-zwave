#include "AVSimpleCommandItem.h"
#include <vector>
#include <string>
#include "Defs.h"

using namespace OpenZWave;

static vector<AVSimpleCommandItem> m_commands;

//-----------------------------------------------------------------------------
// <AVSimpleCommand::AVSimpleCommand>
// AVSimpleCommand constructor
//-----------------------------------------------------------------------------
AVSimpleCommandItem::AVSimpleCommandItem
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
uint16 AVSimpleCommandItem::GetCode()
{
	return m_code;
}

//-----------------------------------------------------------------------------
// <AVSimpleCommand::GetVersion>
// Get version of AV simple command class of command
//-----------------------------------------------------------------------------
uint16 AVSimpleCommandItem::GetVersion()
{
	return m_version;
}

//-----------------------------------------------------------------------------
// <AVSimpleCommand::GetName>
// Get human-friendly name of AV command
//-----------------------------------------------------------------------------
string AVSimpleCommandItem::GetName()
{
	return m_name;
}

//-----------------------------------------------------------------------------
// <AVSimpleCommand::GetDescription>
// Get human-friendly description of AV command
//-----------------------------------------------------------------------------
string AVSimpleCommandItem::GetDescription()
{
	return m_description;
}

//-----------------------------------------------------------------------------
// <AVSimpleCommand::GetCommands>
// Get all available AV commands
//-----------------------------------------------------------------------------
vector<AVSimpleCommandItem> AVSimpleCommandItem::GetCommands()
{
	if (m_commands.size() == 0)
	{
		// Generated code
		m_commands.push_back(AVSimpleCommandItem(1, "Mute", "", 1));
		m_commands.push_back(AVSimpleCommandItem(2, "Volume Down", "Level Down", 1));
		m_commands.push_back(AVSimpleCommandItem(3, "Volume Up", "Level Up", 1));
		m_commands.push_back(AVSimpleCommandItem(4, "Channel Up", "Program Up", 1));
		m_commands.push_back(AVSimpleCommandItem(5, "Channel Down", "Program Down", 1));
		m_commands.push_back(AVSimpleCommandItem(6, "0", "Preset 10", 1));
		m_commands.push_back(AVSimpleCommandItem(7, "1", "Preset 1", 1));
		m_commands.push_back(AVSimpleCommandItem(8, "2", "Preset 2", 1));
		m_commands.push_back(AVSimpleCommandItem(9, "3", "Preset 3", 1));
		m_commands.push_back(AVSimpleCommandItem(10, "4", "Preset 4", 1));
		m_commands.push_back(AVSimpleCommandItem(11, "5", "Preset 5", 1));
		m_commands.push_back(AVSimpleCommandItem(12, "6", "Preset 6", 1));
		m_commands.push_back(AVSimpleCommandItem(13, "7", "Preset 7", 1));
		m_commands.push_back(AVSimpleCommandItem(14, "8", "Preset 8", 1));
		m_commands.push_back(AVSimpleCommandItem(15, "9", "Preset 9", 1));
		m_commands.push_back(AVSimpleCommandItem(16, "Last Channel", "Recall, Previous Channel (WMC)", 1));
		m_commands.push_back(AVSimpleCommandItem(17, "Display", "Info", 1));
		m_commands.push_back(AVSimpleCommandItem(18, "Favorite Channel", "Favorite", 1));
		m_commands.push_back(AVSimpleCommandItem(19, "Play", "", 1));
		m_commands.push_back(AVSimpleCommandItem(20, "Stop", "", 1));
		m_commands.push_back(AVSimpleCommandItem(21, "Pause", "Still", 1));
		m_commands.push_back(AVSimpleCommandItem(22, "Fast Forward", "Search Forward", 1));
		m_commands.push_back(AVSimpleCommandItem(23, "Rewind", "Search Reverse", 1));
		m_commands.push_back(AVSimpleCommandItem(24, "Instant Replay", "Replay", 1));
		m_commands.push_back(AVSimpleCommandItem(25, "Record", "", 1));
		m_commands.push_back(AVSimpleCommandItem(26, "AC3", "Dolby Digital", 1));
		m_commands.push_back(AVSimpleCommandItem(27, "PVR Menu", "Tivo", 1));
		m_commands.push_back(AVSimpleCommandItem(28, "Guide", "EPG", 1));
		m_commands.push_back(AVSimpleCommandItem(29, "Menu", "Settings", 1));
		m_commands.push_back(AVSimpleCommandItem(30, "Menu Up", "Adjust Up", 1));
		m_commands.push_back(AVSimpleCommandItem(31, "Menu Down", "Adjust Down", 1));
		m_commands.push_back(AVSimpleCommandItem(32, "Menu Left", "Cursor Left", 1));
		m_commands.push_back(AVSimpleCommandItem(33, "Menu Right", "Cursor Right", 1));
		m_commands.push_back(AVSimpleCommandItem(34, "Page Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(35, "Page Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(36, "Select", "OK", 1));
		m_commands.push_back(AVSimpleCommandItem(37, "Exit", "", 1));
		m_commands.push_back(AVSimpleCommandItem(38, "Input", "Input Select", 1));
		m_commands.push_back(AVSimpleCommandItem(39, "Power", "Standby", 1));
		m_commands.push_back(AVSimpleCommandItem(40, "Enter Channel", "Channel Enter", 1));
		m_commands.push_back(AVSimpleCommandItem(41, "10", "", 1));
		m_commands.push_back(AVSimpleCommandItem(42, "11", "", 1));
		m_commands.push_back(AVSimpleCommandItem(43, "12", "", 1));
		m_commands.push_back(AVSimpleCommandItem(44, "13", "", 1));
		m_commands.push_back(AVSimpleCommandItem(45, "14", "", 1));
		m_commands.push_back(AVSimpleCommandItem(46, "15", "", 1));
		m_commands.push_back(AVSimpleCommandItem(47, "16", "", 1));
		m_commands.push_back(AVSimpleCommandItem(48, "10", "10+", 1));
		m_commands.push_back(AVSimpleCommandItem(49, "20", "20+", 1));
		m_commands.push_back(AVSimpleCommandItem(50, "100", "", 1));
		m_commands.push_back(AVSimpleCommandItem(51, "-/--", "", 1));
		m_commands.push_back(AVSimpleCommandItem(52, "3-CH", "", 1));
		m_commands.push_back(AVSimpleCommandItem(53, "3D", "Simulated Stereo", 1));
		m_commands.push_back(AVSimpleCommandItem(54, "6-CH Input", "6 Channel", 1));
		m_commands.push_back(AVSimpleCommandItem(55, "A", "", 1));
		m_commands.push_back(AVSimpleCommandItem(56, "Add", "Write", 1));
		m_commands.push_back(AVSimpleCommandItem(57, "Alarm", "", 1));
		m_commands.push_back(AVSimpleCommandItem(58, "AM", "", 1));
		m_commands.push_back(AVSimpleCommandItem(59, "Analog", "", 1));
		m_commands.push_back(AVSimpleCommandItem(60, "Angle", "", 1));
		m_commands.push_back(AVSimpleCommandItem(61, "Antenna", "External", 1));
		m_commands.push_back(AVSimpleCommandItem(62, "Antenna East", "", 1));
		m_commands.push_back(AVSimpleCommandItem(63, "Antenna West", "", 1));
		m_commands.push_back(AVSimpleCommandItem(64, "Aspect", "Size", 1));
		m_commands.push_back(AVSimpleCommandItem(65, "Audio 1", "Audio", 1));
		m_commands.push_back(AVSimpleCommandItem(66, "Audio 2", "", 1));
		m_commands.push_back(AVSimpleCommandItem(67, "Audio 3", "", 1));
		m_commands.push_back(AVSimpleCommandItem(68, "Audio Dubbing", "", 1));
		m_commands.push_back(AVSimpleCommandItem(69, "Audio Level Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(70, "Audio Level Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(71, "Auto/Manual", "", 1));
		m_commands.push_back(AVSimpleCommandItem(72, "Aux 1", "Aux", 1));
		m_commands.push_back(AVSimpleCommandItem(73, "Aux 2", "", 1));
		m_commands.push_back(AVSimpleCommandItem(74, "B", "", 1));
		m_commands.push_back(AVSimpleCommandItem(75, "Back", "Previous Screen", 1));
		m_commands.push_back(AVSimpleCommandItem(76, "Background", "Backlight", 1));
		m_commands.push_back(AVSimpleCommandItem(77, "Balance", "", 1));
		m_commands.push_back(AVSimpleCommandItem(78, "Balance Left", "", 1));
		m_commands.push_back(AVSimpleCommandItem(79, "Balance Right", "", 1));
		m_commands.push_back(AVSimpleCommandItem(80, "Band", "FM/AM", 1));
		m_commands.push_back(AVSimpleCommandItem(81, "Bandwidth", "Wide/Narrow", 1));
		m_commands.push_back(AVSimpleCommandItem(82, "Bass", "", 1));
		m_commands.push_back(AVSimpleCommandItem(83, "Bass Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(84, "Bass Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(85, "Blank", "", 1));
		m_commands.push_back(AVSimpleCommandItem(86, "Breeze Mode", "", 1));
		m_commands.push_back(AVSimpleCommandItem(87, "Bright", "Brighten", 1));
		m_commands.push_back(AVSimpleCommandItem(88, "Brightness", "", 1));
		m_commands.push_back(AVSimpleCommandItem(89, "Brightness Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(90, "Brightness Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(91, "Buy", "", 1));
		m_commands.push_back(AVSimpleCommandItem(92, "C", "", 1));
		m_commands.push_back(AVSimpleCommandItem(93, "Camera", "", 1));
		m_commands.push_back(AVSimpleCommandItem(94, "Category Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(95, "Category Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(96, "Center", "", 1));
		m_commands.push_back(AVSimpleCommandItem(97, "Center Down", "Center Volume Down", 1));
		m_commands.push_back(AVSimpleCommandItem(98, "Center Mode", "", 1));
		m_commands.push_back(AVSimpleCommandItem(99, "Center Up", "Center Volume Up", 1));
		m_commands.push_back(AVSimpleCommandItem(100, "Channel/Program", "C/P", 1));
		m_commands.push_back(AVSimpleCommandItem(101, "Clear", "Cancel", 1));
		m_commands.push_back(AVSimpleCommandItem(102, "Close", "", 1));
		m_commands.push_back(AVSimpleCommandItem(103, "Closed Caption", "CC", 1));
		m_commands.push_back(AVSimpleCommandItem(104, "Cold", "A/C", 1));
		m_commands.push_back(AVSimpleCommandItem(105, "Color", "", 1));
		m_commands.push_back(AVSimpleCommandItem(106, "Color Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(107, "Color Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(108, "Component 1", "RGB 1", 1));
		m_commands.push_back(AVSimpleCommandItem(109, "Component 2", "RGB 2", 1));
		m_commands.push_back(AVSimpleCommandItem(110, "Component 3", "", 1));
		m_commands.push_back(AVSimpleCommandItem(111, "Concert", "", 1));
		m_commands.push_back(AVSimpleCommandItem(112, "Confirm", "Check", 1));
		m_commands.push_back(AVSimpleCommandItem(113, "Continue", "Continuous", 1));
		m_commands.push_back(AVSimpleCommandItem(114, "Contrast", "", 1));
		m_commands.push_back(AVSimpleCommandItem(115, "Contrast Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(116, "Contrast Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(117, "Counter", "", 1));
		m_commands.push_back(AVSimpleCommandItem(118, "Counter Reset", "", 1));
		m_commands.push_back(AVSimpleCommandItem(119, "D", "", 1));
		m_commands.push_back(AVSimpleCommandItem(120, "Day Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(121, "Day Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(122, "Delay", "", 1));
		m_commands.push_back(AVSimpleCommandItem(123, "Delay Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(124, "Delay Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(125, "Delete", "Erase", 1));
		m_commands.push_back(AVSimpleCommandItem(126, "Delimiter", "Sub-Channel", 1));
		m_commands.push_back(AVSimpleCommandItem(127, "Digest", "", 1));
		m_commands.push_back(AVSimpleCommandItem(128, "Digital", "", 1));
		m_commands.push_back(AVSimpleCommandItem(129, "Dim", "Dimmer", 1));
		m_commands.push_back(AVSimpleCommandItem(130, "Direct", "", 1));
		m_commands.push_back(AVSimpleCommandItem(131, "Disarm", "", 1));
		m_commands.push_back(AVSimpleCommandItem(132, "Disc", "", 1));
		m_commands.push_back(AVSimpleCommandItem(133, "Disc 1", "", 1));
		m_commands.push_back(AVSimpleCommandItem(134, "Disc 2", "", 1));
		m_commands.push_back(AVSimpleCommandItem(135, "Disc 3", "", 1));
		m_commands.push_back(AVSimpleCommandItem(136, "Disc 4", "", 1));
		m_commands.push_back(AVSimpleCommandItem(137, "Disc 5", "", 1));
		m_commands.push_back(AVSimpleCommandItem(138, "Disc 6", "", 1));
		m_commands.push_back(AVSimpleCommandItem(139, "Disc Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(140, "Disc Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(141, "Disco", "", 1));
		m_commands.push_back(AVSimpleCommandItem(142, "Edit", "", 1));
		m_commands.push_back(AVSimpleCommandItem(143, "Effect Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(144, "Effect Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(145, "Eject", "Open/Close", 1));
		m_commands.push_back(AVSimpleCommandItem(146, "End", "", 1));
		m_commands.push_back(AVSimpleCommandItem(147, "EQ", "Equalizer", 1));
		m_commands.push_back(AVSimpleCommandItem(148, "Fader", "", 1));
		m_commands.push_back(AVSimpleCommandItem(149, "Fan", "", 1));
		m_commands.push_back(AVSimpleCommandItem(150, "Fan High", "", 1));
		m_commands.push_back(AVSimpleCommandItem(151, "Fan Low", "", 1));
		m_commands.push_back(AVSimpleCommandItem(152, "Fan Medium", "", 1));
		m_commands.push_back(AVSimpleCommandItem(153, "Fan Speed", "", 1));
		m_commands.push_back(AVSimpleCommandItem(154, "Fastext Blue", "", 1));
		m_commands.push_back(AVSimpleCommandItem(155, "Fastext Green", "", 1));
		m_commands.push_back(AVSimpleCommandItem(156, "Fastext Purple", "", 1));
		m_commands.push_back(AVSimpleCommandItem(157, "Fastext Red", "", 1));
		m_commands.push_back(AVSimpleCommandItem(158, "Fastext White", "", 1));
		m_commands.push_back(AVSimpleCommandItem(159, "Fastext Yellow", "", 1));
		m_commands.push_back(AVSimpleCommandItem(160, "Favorite Channel Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(161, "Favorite Channel Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(162, "Finalize", "", 1));
		m_commands.push_back(AVSimpleCommandItem(163, "Fine Tune", "", 1));
		m_commands.push_back(AVSimpleCommandItem(164, "Flat", "", 1));
		m_commands.push_back(AVSimpleCommandItem(165, "FM", "", 1));
		m_commands.push_back(AVSimpleCommandItem(166, "Focus Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(167, "Focus Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(168, "Freeze", "", 1));
		m_commands.push_back(AVSimpleCommandItem(169, "Front", "", 1));
		m_commands.push_back(AVSimpleCommandItem(170, "Game", "", 1));
		m_commands.push_back(AVSimpleCommandItem(171, "GoTo", "Index Search", 1));
		m_commands.push_back(AVSimpleCommandItem(172, "Hall", "", 1));
		m_commands.push_back(AVSimpleCommandItem(173, "Heat", "", 1));
		m_commands.push_back(AVSimpleCommandItem(174, "Help", "", 1));
		m_commands.push_back(AVSimpleCommandItem(175, "Home", "", 1));
		m_commands.push_back(AVSimpleCommandItem(176, "Index", "VISS", 1));
		m_commands.push_back(AVSimpleCommandItem(177, "Index Forward", "", 1));
		m_commands.push_back(AVSimpleCommandItem(178, "Index Reverse", "", 1));
		m_commands.push_back(AVSimpleCommandItem(179, "Interactive", "Planner", 1));
		m_commands.push_back(AVSimpleCommandItem(180, "Intro Scan", "", 1));
		m_commands.push_back(AVSimpleCommandItem(181, "Jazz", "", 1));
		m_commands.push_back(AVSimpleCommandItem(182, "Karaoke", "", 1));
		m_commands.push_back(AVSimpleCommandItem(183, "Keystone", "", 1));
		m_commands.push_back(AVSimpleCommandItem(184, "Keystone Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(185, "Keystone Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(186, "Language", "SAP", 1));
		m_commands.push_back(AVSimpleCommandItem(187, "Left Click", "", 1));
		m_commands.push_back(AVSimpleCommandItem(188, "Level", "Volume", 1));
		m_commands.push_back(AVSimpleCommandItem(189, "Light", "Lamp", 1));
		m_commands.push_back(AVSimpleCommandItem(190, "List", "My Shows", 1));
		m_commands.push_back(AVSimpleCommandItem(191, "Live TV", "Return to Live", 1));
		m_commands.push_back(AVSimpleCommandItem(192, "Local/Dx", "", 1));
		m_commands.push_back(AVSimpleCommandItem(193, "Loudness", "", 1));
		m_commands.push_back(AVSimpleCommandItem(194, "Mail", "Email", 1));
		m_commands.push_back(AVSimpleCommandItem(195, "Mark", "Bookmark", 1));
		m_commands.push_back(AVSimpleCommandItem(196, "Memory Recall", "", 1));
		m_commands.push_back(AVSimpleCommandItem(197, "Monitor", "Tape Monitor", 1));
		m_commands.push_back(AVSimpleCommandItem(198, "Movie", "", 1));
		m_commands.push_back(AVSimpleCommandItem(199, "Multi Room", "", 1));
		m_commands.push_back(AVSimpleCommandItem(200, "Music", "TV/Radio, My Music (WMC)", 1));
		m_commands.push_back(AVSimpleCommandItem(201, "Music Scan", "Memory Scan", 1));
		m_commands.push_back(AVSimpleCommandItem(202, "Natural", "", 1));
		m_commands.push_back(AVSimpleCommandItem(203, "Night", "", 1));
		m_commands.push_back(AVSimpleCommandItem(204, "Noise Reduction", "Dolby NR", 1));
		m_commands.push_back(AVSimpleCommandItem(205, "Normalize", "Personal Preference", 1));
		m_commands.push_back(AVSimpleCommandItem(206, "Discrete input Cable", "CATV", 1));
		m_commands.push_back(AVSimpleCommandItem(207, "Discrete input CD 1", "CD", 1));
		m_commands.push_back(AVSimpleCommandItem(208, "Discrete input CD 2", "CDR", 1));
		m_commands.push_back(AVSimpleCommandItem(209, "Discrete input CDR", "Compact Disc Recorder", 1));
		m_commands.push_back(AVSimpleCommandItem(210, "Discrete input DAT", "Digital Audio Tape", 1));
		m_commands.push_back(AVSimpleCommandItem(211, "Discrete input DVD", "Digital Video Disk", 1));
		m_commands.push_back(AVSimpleCommandItem(212, "Discrete input DVI", "Digital Video Interface", 1));
		m_commands.push_back(AVSimpleCommandItem(213, "Discrete input HDTV", "", 1));
		m_commands.push_back(AVSimpleCommandItem(214, "Discrete input LD", "Laser Disc", 1));
		m_commands.push_back(AVSimpleCommandItem(215, "Discrete input MD", "Mini Disc", 1));
		m_commands.push_back(AVSimpleCommandItem(216, "Discrete input PC", "Personal Computer", 1));
		m_commands.push_back(AVSimpleCommandItem(217, "Discrete input PVR", "Personal Video Recorder", 1));
		m_commands.push_back(AVSimpleCommandItem(218, "Discrete input TV", "", 1));
		m_commands.push_back(AVSimpleCommandItem(219, "Discrete input TV/VCR", "TV/DVD", 1));
		m_commands.push_back(AVSimpleCommandItem(220, "Discrete input VCR", "", 1));
		m_commands.push_back(AVSimpleCommandItem(221, "One Touch Playback", "OTPB", 1));
		m_commands.push_back(AVSimpleCommandItem(222, "One Touch Record", "OTR", 1));
		m_commands.push_back(AVSimpleCommandItem(223, "Open", "", 1));
		m_commands.push_back(AVSimpleCommandItem(224, "Optical", "", 1));
		m_commands.push_back(AVSimpleCommandItem(225, "Options", "", 1));
		m_commands.push_back(AVSimpleCommandItem(226, "Orchestra", "", 1));
		m_commands.push_back(AVSimpleCommandItem(227, "PAL/NTSC", "System Select", 1));
		m_commands.push_back(AVSimpleCommandItem(228, "Parental Lock", "Parental Control", 1));
		m_commands.push_back(AVSimpleCommandItem(229, "PBC", "Playback Control", 1));
		m_commands.push_back(AVSimpleCommandItem(230, "Phono", "", 1));
		m_commands.push_back(AVSimpleCommandItem(231, "Photos", "Pictures, My Pictures (WMC)", 1));
		m_commands.push_back(AVSimpleCommandItem(232, "Picture Menu", "Picture Adjust", 1));
		m_commands.push_back(AVSimpleCommandItem(233, "Picture Mode", "Smart Picture", 1));
		m_commands.push_back(AVSimpleCommandItem(234, "Picture Mute", "", 1));
		m_commands.push_back(AVSimpleCommandItem(235, "PIP Channel Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(236, "PIP Channel Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(237, "PIP Freeze", "", 1));
		m_commands.push_back(AVSimpleCommandItem(238, "PIP Input", "PIP Mode", 1));
		m_commands.push_back(AVSimpleCommandItem(239, "PIP Move", "PIP Position", 1));
		m_commands.push_back(AVSimpleCommandItem(240, "PIP Off", "", 1));
		m_commands.push_back(AVSimpleCommandItem(241, "PIP On", "PIP", 1));
		m_commands.push_back(AVSimpleCommandItem(242, "PIP Size", "", 1));
		m_commands.push_back(AVSimpleCommandItem(243, "PIP Split", "Multi Screen", 1));
		m_commands.push_back(AVSimpleCommandItem(244, "PIP Swap", "PIP Exchange", 1));
		m_commands.push_back(AVSimpleCommandItem(245, "Play Mode", "", 1));
		m_commands.push_back(AVSimpleCommandItem(246, "Play Reverse", "", 1));
		m_commands.push_back(AVSimpleCommandItem(247, "Power Off", "", 1));
		m_commands.push_back(AVSimpleCommandItem(248, "Power On", "", 1));
		m_commands.push_back(AVSimpleCommandItem(249, "PPV", "Pay Per View", 1));
		m_commands.push_back(AVSimpleCommandItem(250, "Preset", "", 1));
		m_commands.push_back(AVSimpleCommandItem(251, "Program", "Program Memory", 1));
		m_commands.push_back(AVSimpleCommandItem(252, "Progressive Scan", "Progressive", 1));
		m_commands.push_back(AVSimpleCommandItem(253, "ProLogic", "Dolby Prologic", 1));
		m_commands.push_back(AVSimpleCommandItem(254, "PTY", "Audio Program Type", 1));
		m_commands.push_back(AVSimpleCommandItem(255, "Quick Skip", "Commercial Skip", 1));
		m_commands.push_back(AVSimpleCommandItem(256, "Random", "Shuffle", 1));
		m_commands.push_back(AVSimpleCommandItem(257, "RDS", "Radio Data System", 1));
		m_commands.push_back(AVSimpleCommandItem(258, "Rear", "", 1));
		m_commands.push_back(AVSimpleCommandItem(259, "Rear Volume Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(260, "Rear Volume Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(261, "Record Mute", "", 1));
		m_commands.push_back(AVSimpleCommandItem(262, "Record Pause", "", 1));
		m_commands.push_back(AVSimpleCommandItem(263, "Repeat", "", 1));
		m_commands.push_back(AVSimpleCommandItem(264, "Repeat A-B", "", 1));
		m_commands.push_back(AVSimpleCommandItem(265, "Resume", "", 1));
		m_commands.push_back(AVSimpleCommandItem(266, "RGB", "Red Green Blue Component Video", 1));
		m_commands.push_back(AVSimpleCommandItem(267, "Right Click", "", 1));
		m_commands.push_back(AVSimpleCommandItem(268, "Rock", "", 1));
		m_commands.push_back(AVSimpleCommandItem(269, "Rotate Left", "", 1));
		m_commands.push_back(AVSimpleCommandItem(270, "Rotate Right", "", 1));
		m_commands.push_back(AVSimpleCommandItem(271, "SAT", "Sky", 1));
		m_commands.push_back(AVSimpleCommandItem(272, "Scan", "Channel Scan", 1));
		m_commands.push_back(AVSimpleCommandItem(273, "Scart", "", 1));
		m_commands.push_back(AVSimpleCommandItem(274, "Scene", "", 1));
		m_commands.push_back(AVSimpleCommandItem(275, "Scroll", "", 1));
		m_commands.push_back(AVSimpleCommandItem(276, "Services", "", 1));
		m_commands.push_back(AVSimpleCommandItem(277, "Setup Menu", "Setup", 1));
		m_commands.push_back(AVSimpleCommandItem(278, "Sharp", "", 1));
		m_commands.push_back(AVSimpleCommandItem(279, "Sharpness", "", 1));
		m_commands.push_back(AVSimpleCommandItem(280, "Sharpness Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(281, "Sharpness Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(282, "Side A/B", "", 1));
		m_commands.push_back(AVSimpleCommandItem(283, "Skip Forward", "Next", 1));
		m_commands.push_back(AVSimpleCommandItem(284, "Skip Reverse", "Previous", 1));
		m_commands.push_back(AVSimpleCommandItem(285, "Sleep", "Off Timer", 1));
		m_commands.push_back(AVSimpleCommandItem(286, "Slow", "", 1));
		m_commands.push_back(AVSimpleCommandItem(287, "Slow Forward", "", 1));
		m_commands.push_back(AVSimpleCommandItem(288, "Slow Reverse", "", 1));
		m_commands.push_back(AVSimpleCommandItem(289, "Sound Menu", "Audio Menu", 1));
		m_commands.push_back(AVSimpleCommandItem(290, "Sound Mode", "Smart Sound", 1));
		m_commands.push_back(AVSimpleCommandItem(291, "Speed", "Record Speed", 1));
		m_commands.push_back(AVSimpleCommandItem(292, "Speed Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(293, "Speed Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(294, "Sports", "Digital Surround Processing", 1));
		m_commands.push_back(AVSimpleCommandItem(295, "Stadium", "", 1));
		m_commands.push_back(AVSimpleCommandItem(296, "Start", "", 1));
		m_commands.push_back(AVSimpleCommandItem(297, "Start ID Erase", "Erase", 1));
		m_commands.push_back(AVSimpleCommandItem(298, "Start ID Renumber", "Renumber", 1));
		m_commands.push_back(AVSimpleCommandItem(299, "Start ID Write", "Write", 1));
		m_commands.push_back(AVSimpleCommandItem(300, "Step", "", 1));
		m_commands.push_back(AVSimpleCommandItem(301, "Stereo/Mono", "L/R", 1));
		m_commands.push_back(AVSimpleCommandItem(302, "Still Forward", "Frame Advance", 1));
		m_commands.push_back(AVSimpleCommandItem(303, "Still Reverse", "Frame Reverse", 1));
		m_commands.push_back(AVSimpleCommandItem(304, "Subtitle", "Subtitle On-Off", 1));
		m_commands.push_back(AVSimpleCommandItem(305, "Subwoofer Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(306, "Subwoofer Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(307, "Super Bass", "Bass Boost", 1));
		m_commands.push_back(AVSimpleCommandItem(308, "Surround", "", 1));
		m_commands.push_back(AVSimpleCommandItem(309, "Surround Mode", "Sound Field", 1));
		m_commands.push_back(AVSimpleCommandItem(310, "S-Video", "", 1));
		m_commands.push_back(AVSimpleCommandItem(311, "Sweep", "Oscillate", 1));
		m_commands.push_back(AVSimpleCommandItem(312, "Synchro Record", "CD Synchro", 1));
		m_commands.push_back(AVSimpleCommandItem(313, "Tape 1", "Deck 1", 1));
		m_commands.push_back(AVSimpleCommandItem(314, "Tape 1-2", "Deck 1-2", 1));
		m_commands.push_back(AVSimpleCommandItem(315, "Tape 2", "Deck 2", 1));
		m_commands.push_back(AVSimpleCommandItem(316, "Temperature Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(317, "Temperature Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(318, "Test Tone", "", 1));
		m_commands.push_back(AVSimpleCommandItem(319, "Text", "Teletext", 1));
		m_commands.push_back(AVSimpleCommandItem(320, "Text Expand", "", 1));
		m_commands.push_back(AVSimpleCommandItem(321, "Text Hold", "", 1));
		m_commands.push_back(AVSimpleCommandItem(322, "Text Index", "", 1));
		m_commands.push_back(AVSimpleCommandItem(323, "Text Mix", "", 1));
		m_commands.push_back(AVSimpleCommandItem(324, "Text Off", "", 1));
		m_commands.push_back(AVSimpleCommandItem(325, "Text Reveal", "", 1));
		m_commands.push_back(AVSimpleCommandItem(326, "Text Subpage", "", 1));
		m_commands.push_back(AVSimpleCommandItem(327, "Text Timed Page", "", 1));
		m_commands.push_back(AVSimpleCommandItem(328, "Text Update", "Text Cancel", 1));
		m_commands.push_back(AVSimpleCommandItem(329, "Theater", "Cinema EQ", 1));
		m_commands.push_back(AVSimpleCommandItem(330, "Theme", "Category Select", 1));
		m_commands.push_back(AVSimpleCommandItem(331, "Thumbs Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(332, "Thumbs Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(333, "Tilt Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(334, "Tilt Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(335, "Time", "Clock", 1));
		m_commands.push_back(AVSimpleCommandItem(336, "Timer", "", 1));
		m_commands.push_back(AVSimpleCommandItem(337, "Timer Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(338, "Timer Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(339, "Tint", "", 1));
		m_commands.push_back(AVSimpleCommandItem(340, "Tint Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(341, "Tint Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(342, "Title", "Top Menu", 1));
		m_commands.push_back(AVSimpleCommandItem(343, "Track", "Chapter", 1));
		m_commands.push_back(AVSimpleCommandItem(344, "Tracking", "", 1));
		m_commands.push_back(AVSimpleCommandItem(345, "Tracking Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(346, "Tracking Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(347, "Treble", "", 1));
		m_commands.push_back(AVSimpleCommandItem(348, "Treble Down", "", 1));
		m_commands.push_back(AVSimpleCommandItem(349, "Treble Up", "", 1));
		m_commands.push_back(AVSimpleCommandItem(350, "Tune Down", "Audio Tune Down", 1));
		m_commands.push_back(AVSimpleCommandItem(351, "Tune Up", "Audio Tune Up", 1));
		m_commands.push_back(AVSimpleCommandItem(352, "Tuner", "", 1));
		m_commands.push_back(AVSimpleCommandItem(353, "VCR Plus+", "Showview", 1));
		m_commands.push_back(AVSimpleCommandItem(354, "Video 1", "A/V 1", 1));
		m_commands.push_back(AVSimpleCommandItem(355, "Video 2", "A/V 2", 1));
		m_commands.push_back(AVSimpleCommandItem(356, "Video 3", "A/V 3", 1));
		m_commands.push_back(AVSimpleCommandItem(357, "Video 4", "A/V 4", 1));
		m_commands.push_back(AVSimpleCommandItem(358, "Video 5", "A/V 5", 1));
		m_commands.push_back(AVSimpleCommandItem(359, "View", "", 1));
		m_commands.push_back(AVSimpleCommandItem(360, "Voice", "Vocals", 1));
		m_commands.push_back(AVSimpleCommandItem(361, "Zoom", "Magnify", 1));
		m_commands.push_back(AVSimpleCommandItem(362, "Zoom In", "Zoom Up", 1));
		m_commands.push_back(AVSimpleCommandItem(363, "Zoom Out", "Zoom Down", 1));
		m_commands.push_back(AVSimpleCommandItem(364, "eHome", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommandItem(365, "Details", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommandItem(366, "DVD Menu", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommandItem(367, "My TV", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommandItem(368, "Recorded TV", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommandItem(369, "My Videos", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommandItem(370, "DVD Angle", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommandItem(371, "DVD Audio", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommandItem(372, "DVD Subtitle", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommandItem(373, "Radio", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommandItem(374, "#", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommandItem(375, "*", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommandItem(376, "OEM 1", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommandItem(377, "OEM 2", "(Windows Media Center)", 2));
		m_commands.push_back(AVSimpleCommandItem(378, "Info", "Used to request information", 3));
		m_commands.push_back(AVSimpleCommandItem(379, "CAPS NUM", "Switch between numeric and alpha (Shift)", 3));
		m_commands.push_back(AVSimpleCommandItem(380, "TV MODE", "Cycles through video output modes/resolutions", 3));
		m_commands.push_back(AVSimpleCommandItem(381, "SOURCE", "Displays the possible sources for the playback. [NFS, ext., USB, UPnP,…]", 3));
		m_commands.push_back(AVSimpleCommandItem(382, "FILEMODE", "File manipulation. Add/remove to list, create folder, rename file,…", 3));
		m_commands.push_back(AVSimpleCommandItem(383, "Time Seek", "This seeks to time position. Used for DVD/CD/others", 3));
		m_commands.push_back(AVSimpleCommandItem(384, "Mouse enable", "Mouse pointer enable", 4));
		m_commands.push_back(AVSimpleCommandItem(385, "Mouse disable", "Mouse pointer disable", 4));
		m_commands.push_back(AVSimpleCommandItem(386, "VOD", "Video on demand", 4));
		m_commands.push_back(AVSimpleCommandItem(387, "Thumbs Up", "Thumbs up for positive feedback in GUI", 4));
		m_commands.push_back(AVSimpleCommandItem(388, "Thumbs Down", "Thumbs down for negative feedback in GUI", 4));
		m_commands.push_back(AVSimpleCommandItem(389, "Apps", "Application selection/launch", 4));
		m_commands.push_back(AVSimpleCommandItem(390, "Mouse toggle", "Will toggle a mouse pointer between on and off", 4));
		m_commands.push_back(AVSimpleCommandItem(391, "TV Mode", "Will direct an AV device to go the TV mode (the mode is configured on the device)", 4));
		m_commands.push_back(AVSimpleCommandItem(392, "DVD Mode", "Will direct an AV device to go the DVD mode (the mode is configured on the device)", 4));
		m_commands.push_back(AVSimpleCommandItem(393, "STB Mode", "Will direct an AV device to go the STB mode (the mode is configured on the device)", 4));
		m_commands.push_back(AVSimpleCommandItem(394, "AUX Mode", "Will direct an AV device to go the AUX mode (the mode is configured on the device)", 4));
		m_commands.push_back(AVSimpleCommandItem(395, "BluRay Mode", "Will direct an AV device to go the BluRay mode (the mode is configured on the device)", 4));
		m_commands.push_back(AVSimpleCommandItem(404, "Standby 1", "Used for AV devices that support multiple standby mode. Power ON should be used to turn on the device", 4));
		m_commands.push_back(AVSimpleCommandItem(405, "Standby 2", "Used for AV devices that support multiple standby mode. Power ON should be used to turn on the device", 4));
		m_commands.push_back(AVSimpleCommandItem(406, "Standby 3", "Used for AV devices that support multiple standby mode. Power ON should be used to turn on the device", 4));
		m_commands.push_back(AVSimpleCommandItem(407, "HDMI 1", "Discrete command used to set an AV device to HDMI input 1", 4));
		m_commands.push_back(AVSimpleCommandItem(408, "HDMI 2", "Discrete command used to set an AV device to HDMI input 2", 4));
		m_commands.push_back(AVSimpleCommandItem(409, "HDMI 3", "Discrete command used to set an AV device to HDMI input 3", 4));
		m_commands.push_back(AVSimpleCommandItem(410, "HDMI 4", "Discrete command used to set an AV device to HDMI input 4", 4));
		m_commands.push_back(AVSimpleCommandItem(411, "HDMI 5", "Discrete command used to set an AV device to HDMI input 5", 4));
		m_commands.push_back(AVSimpleCommandItem(412, "HDMI 6", "Discrete command used to set an AV device to HDMI input 6", 4));
		m_commands.push_back(AVSimpleCommandItem(413, "HDMI 7", "Discrete command used to set an AV device to HDMI input 7", 4));
		m_commands.push_back(AVSimpleCommandItem(414, "HDMI 8", "Discrete command used to set an AV device to HDMI input 8", 4));
		m_commands.push_back(AVSimpleCommandItem(415, "HDMI 9", "Discrete command used to set an AV device to HDMI input 9", 4));
		m_commands.push_back(AVSimpleCommandItem(416, "USB 1", "Discrete command used to set an AV device to USB input 1", 4));
		m_commands.push_back(AVSimpleCommandItem(417, "USB 2", "Discrete command used to set an AV device to USB input 2", 4));
		m_commands.push_back(AVSimpleCommandItem(418, "USB 3", "Discrete command used to set an AV device to USB input 3", 4));
		m_commands.push_back(AVSimpleCommandItem(419, "USB 4", "Discrete command used to set an AV device to USB input 4", 4));
		m_commands.push_back(AVSimpleCommandItem(420, "USB 5", "Discrete command used to set an AV device to USB input 5", 4));
		m_commands.push_back(AVSimpleCommandItem(421, "ZOOM - 4:3 Normal", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(AVSimpleCommandItem(422, "ZOOM - 4:3 Zoom", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(AVSimpleCommandItem(423, "ZOOM - 16:9 Normal", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(AVSimpleCommandItem(424, "ZOOM - 16:9 Zoom", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(AVSimpleCommandItem(425, "ZOOM - 16:9 Wide 1", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(AVSimpleCommandItem(426, "ZOOM 16:9 Wide 2", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(AVSimpleCommandItem(427, "ZOOM 16:9 Wide 3", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(AVSimpleCommandItem(428, "ZOOM 16:9 Cinema", "Discrete commands that is used to set a TV a direct Zoom mode", 4));
		m_commands.push_back(AVSimpleCommandItem(429, "ZOOM Default", "Discrete commands that is used to set a TV the default Zoom mode", 4));
		m_commands.push_back(AVSimpleCommandItem(432, "Auto Zoom", "Will set Zoom mode automatically", 4));
		m_commands.push_back(AVSimpleCommandItem(433, "ZOOM - Set as Default Zoom", "Will set the current active Zoom level to default", 4));
		m_commands.push_back(AVSimpleCommandItem(434, "Mute ON", "Discrete Mute ON command", 4));
		m_commands.push_back(AVSimpleCommandItem(435, "Mute OFF", "Discrete Mute OFF command", 4));
		m_commands.push_back(AVSimpleCommandItem(436, "AUDIO Mode - AUDYSSEY AUDIO OFF", "Discrete Audio mode for Audussey audio processing (Off) ", 4));
		m_commands.push_back(AVSimpleCommandItem(437, "AUDIO Mode - AUDYSSEY AUDIO LO", "Discrete Audio mode for Audussey audio processing (Low) ", 4));
		m_commands.push_back(AVSimpleCommandItem(438, "AUDIO Mode - AUDYSSEY AUDIO MED", "Discrete Audio mode for Audussey audio processing (Medium) ", 4));
		m_commands.push_back(AVSimpleCommandItem(439, "AUDIO Mode - AUDYSSEY AUDIO HI", "Discrete Audio mode for Audussey audio processing (High) ", 4));
		m_commands.push_back(AVSimpleCommandItem(442, "AUDIO Mode - SRS SURROUND ON", "Discrete Audio mode for SRS audio processing", 4));
		m_commands.push_back(AVSimpleCommandItem(443, "AUDIO Mode - SRS SURROUND OFF", "Discrete Audio mode for SRS audio processing", 4));
		m_commands.push_back(AVSimpleCommandItem(447, "Picture Mode - Home", "Discrete picture for TVs", 4));
		m_commands.push_back(AVSimpleCommandItem(448, "Picture Mode - Retail", "Discrete picture for TVs", 4));
		m_commands.push_back(AVSimpleCommandItem(449, "Picture Mode - Vivid", "Discrete picture for TVs", 4));
		m_commands.push_back(AVSimpleCommandItem(450, "Picture Mode - Standard", "Discrete picture for TVs", 4));
		m_commands.push_back(AVSimpleCommandItem(451, "Picture Mode - Theater", "Discrete picture for TVs", 4));
		m_commands.push_back(AVSimpleCommandItem(452, "Picture Mode - Sports", "Discrete picture for TVs", 4));
		m_commands.push_back(AVSimpleCommandItem(453, "Picture Mode - Energy savings ", "Discrete picture for TVs", 4));
		m_commands.push_back(AVSimpleCommandItem(454, "Picture Mode - Custom", "Discrete picture for TVs", 4));
		m_commands.push_back(AVSimpleCommandItem(455, "Cool", "Discrete picture temperature adjustments", 4));
		m_commands.push_back(AVSimpleCommandItem(456, "Medium", "Discrete picture temperature adjustments", 4));
		m_commands.push_back(AVSimpleCommandItem(457, "Warm_D65", "Discrete picture temperature adjustments", 4));
		m_commands.push_back(AVSimpleCommandItem(458, "CC ON", "Discrete Closed caption commands", 4));
		m_commands.push_back(AVSimpleCommandItem(459, "CC OFF", "Discrete Closed caption commands", 4));
		m_commands.push_back(AVSimpleCommandItem(460, "Video Mute ON", "Discrete Video mute command", 4));
		m_commands.push_back(AVSimpleCommandItem(461, "Video Mute OFF", "Discrete Video mute command", 4));
		m_commands.push_back(AVSimpleCommandItem(462, "Next Event", "Go to next state or event ", 4));
		m_commands.push_back(AVSimpleCommandItem(463, "Previous Event", "Go to previous state or event", 4));
		m_commands.push_back(AVSimpleCommandItem(464, "CEC device list", "Brings up the CES device list", 4));
		m_commands.push_back(AVSimpleCommandItem(465, "MTS SAP", "Secondary Audio programming", 4));
	}
	return m_commands;
}