/***********************************************************************************************
created: 		2022-01-20

author:			chensong

purpose:		protocol
输赢不重要，答案对你们有什么意义才重要。

光阴者，百代之过客也，唯有奋力奔跑，方能生风起时，是时代造英雄，英雄存在于时代。或许世人道你轻狂，可你本就年少啊。 看护好，自己的理想和激情。


我可能会遇到很多的人，听他们讲好2多的故事，我来写成故事或编成歌，用我学来的各种乐器演奏它。
然后还可能在一个国家遇到一个心仪我的姑娘，她可能会被我帅气的外表捕获，又会被我深邃的内涵吸引，在某个下雨的夜晚，她会全身淋透然后要在我狭小的住处换身上的湿衣服。
3小时候后她告诉我她其实是这个国家的公主，她愿意向父皇求婚。我不得已告诉她我是穿越而来的男主角，我始终要回到自己的世界。
然后我的身影慢慢消失，我看到她眼里的泪水，心里却没有任何痛苦，我才知道，原来我的心被丢掉了，我游历全世界的原因，就是要找回自己的本心。
于是我开始有意寻找各种各样失去心的人，我变成一块砖头，一颗树，一滴水，一朵白云，去听大家为什么会失去自己的本心。
我发现，刚出生的宝宝，本心还在，慢慢的，他们的本心就会消失，收到了各种黑暗之光的侵蚀。
从一次争论，到嫉妒和悲愤，还有委屈和痛苦，我看到一只只无形的手，把他们的本心扯碎，蒙蔽，偷走，再也回不到主人都身边。
我叫他本心猎手。他可能是和宇宙同在的级别 但是我并不害怕，我仔细回忆自己平淡的一生 寻找本心猎手的痕迹。
沿着自己的回忆，一个个的场景忽闪而过，最后发现，我的本心，在我写代码的时候，会回来。
安静，淡然，代码就是我的一切，写代码就是我本心回归的最好方式，我还没找到本心猎手，但我相信，顺着这个线索，我一定能顺藤摸瓜，把他揪出来。
************************************************************************************************/
#ifndef _C_PROTOCOL_H_
#define _C_PROTOCOL_H_
#include "cnet_types.h"
#include "cassertion_macros.h"
#include "api/data_channel_interface.h"
#include <vector>

namespace chen {

	enum  EToStreamMsg :uint8
	{
		/**********************************************************************/

		/*
		* Control Messages. Range = 0..49.
		*/

		IFrameRequest = 0,
		RequestQualityControl = 1,
		MaxFpsRequest = 2,
		AverageBitrateRequest = 3,
		StartStreaming = 4,
		StopStreaming = 5,
		LatencyTest = 6,
		RequestInitialSettings = 7,

		/**********************************************************************/

		/*
		* Input Messages. Range = 50..89.
		*/

		// Generic Input Messages. Range = 50..59.
		UIInteraction = 50,
		Command = 51,
		CommandMessage = 52,


		// Keyboard Input Message. Range = 60..69.
		KeyDown = 60,
		KeyUp = 61,
		KeyPress = 62,

		// Mouse Input Messages. Range = 70..79.
		MouseEnter = 70,
		MouseLeave = 71,
		MouseDown = 72,
		MouseUp = 73,
		MouseMove = 74,
		MouseWheel = 75,
		MouseDoubleClick = 76,
		// Touch Input Messages. Range = 80..89.
		TouchStart = 80,
		Touchend = 81,
		TouchMove = 82,

		// Gamepad Input Messages. Range = 90..99
		GamepadButtonPressed = 90,
		GamepadButtonReleased = 91,
		GamepadAnalog = 92,

		/**********************************************************************/

		/*
		* Ensure Count is the final entry.
		*/
		Count

		/**********************************************************************/
	};
	//! Messages that can be sent to the webrtc players
	//enum   EToPlayerMsg : uint8 
	//{
	//	QualityControlOwnership = 0,
	//	Response = 1,
	//	Command = 2,
	//	FreezeFrame = 3,
	//	UnfreezeFrame = 4,
	//	VideoEncoderAvgQP = 5, // average Quantisation Parameter value of Video Encoder, roughly depicts video encoding quality
	//	//LatencyTest = 6,
	//	InitialSettings = 7
	//};



	template<typename T>
	static const T& ParseBuffer(const uint8*& Data, uint32& Size)
	{
		checkf(sizeof(T) <= Size, TEXT("%d - %d"), sizeof(T), Size);
		const T& Value = *reinterpret_cast<const T*>(Data);
		Data += sizeof(T);
		Size -= sizeof(T);
		return Value;
	}

	static std::vector<TCHAR> ParseString(const webrtc::DataBuffer& Buffer, const size_t Offset = 0)
	{
		std::vector<TCHAR> Res;
		if (Buffer.data.size() > Offset) {
			size_t StringLength = (Buffer.data.size() - Offset) / sizeof(TCHAR); 
			Res.reserve(StringLength + 1);
			Res.shrink_to_fit();
			memcpy(Res.data(), Buffer.data.data() + Offset, StringLength * sizeof(TCHAR));
			Res[StringLength] = 0;
		}
		return Res;
	}


	// 序列化的操作
#define GET(Type, Var) Type Var = ParseBuffer<Type>(Data, Size)

}
#endif // _C_PROTOCOL_H_