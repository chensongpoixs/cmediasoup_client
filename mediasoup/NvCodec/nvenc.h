/***********************************************************************************************
created: 		2020-2-13

author:			chensong

purpose:		nvenc  

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
ËÙÂÊ¿ØÖÆ

速率控制

NVENC 支持多种速率控制模式，并通过结构NV_ENC_INITIALIZE_PARAMS::encodeConfig::rcParams提供对与速率控制算法相关的各种参数的控制 . 速率控制算法在 NVENC 固件中实现。
NVENC 支持以下速率控制模式：
恒定比特率 (CBR)：恒定比特率设置rateControlMode 为 NV_ENC_PARAMS_RC_CBR. 在这种模式下，只有averageBitRate需要并用作速率控制算法的目标输出比特率。客户端可以使用NV_ENC_RC_PARAMS::lowDelayKeyFrameScale来控制 I 帧与 P 帧的比率，如果 I 帧最终生成大量比特，这对于避免信道拥塞很有用。设置NV_ENC_CONFIG_H264/ NV_ENC_CONFIG_HEVC::enableFillerDataInsertion = 1以防需要严格遵守比特率。
可变比特率 (VBR)：可变比特率设置rateControlMode 为 NV_ENC_PARAMS_RC_VBR。 编码器尝试符合平均比特率averageBitRate，并在编码的任何时间不内超过maxBitRate。在这种模式下，averageBitRate必须指定。如果maxBitRate未指定，NVENC 会将其设置为内部确定的默认值。为了更好的控制，建议客户端同时指定这两个参数maxBitRate和averageBitRate。
恒定 QP：此模式设置rateControlMode为 NV_ENC_PARAMS_RC_CONSTQP. 在这种模式下，整个帧使用指定的 QP (NV_ENC_RC_PARAMS::constQP)编码。
目标质量：此模式设置rateControlMode为VBR 和所需的目标质量targetQuality. 此目标质量的范围是 0 到 51（视频编解码器 SDK 8.0 及更高版本也支持小数值）。在这种模式下，编码器通过maxBitRate中指定的比特率参数的变化来保持每帧的质量. 因此，最终的平均比特率可能会因正在编码的视频内容而有很大差异。如果maxBitRate未指定，编码器将根据需要使用尽可能多的位来实现目标质量。然而，如果maxBitRate设置后，它将形成实际比特率的上限。如果maxBitRate在此模式下设置过低，比特率可能会受到限制，导致可能无法实现所需的目标质量。


 

$(CUDA_PATH)\lib\x64\cuda.lib
$(CUDA_PATH)\lib\x64\cudadevrt.lib
$(CUDA_PATH)\lib\x64\cudart.lib
$(CUDA_PATH)\lib\x64\cudart_static.lib
$(CUDA_PATH)\lib\x64\OpenCL.lib


************************************************************************************************/
 
#ifndef _NVIDIA_ENCODER_H
#define _NVIDIA_ENCODER_H

#include "encoder_info.h"

// Video-Codec-SDK Version: 8.2
#include "NvEncoder/NvEncoderD3D11.h" 
namespace chen {
extern struct encoder_info nvenc_info;
extern uint32 g_gpu_index;;
extern DXGI_FORMAT g_dxgi_format;
}
#endif
