/***********************************************************************************************
created: 		2022-01-20

author:			chensong

purpose:		assertion macros
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
#include "ctransport.h"
#include "clog.h"
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/create_peerconnection_factory.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <rtc_base/ssl_adapter.h>
#include "sdptransform.hpp"
#include "sdp/Utils.hpp"
#include "ortc.hpp"
#include "cproducer.h"
#include <memory>
#include "cclient.h"
#include "external_video_encoder_factory.h"
#include "cdataconsumer.h"
#include "csession_description.h"
#include "pc/video_track_source.h"
#include <system_wrappers/include/field_trial.h>

namespace chen {

	

	
	ctransport::ctransport( std::string transport_id, cclient* ptr):m_transport_id ( transport_id), m_client_ptr(ptr) , m_streamId(0)
	{}
	bool ctransport::init( const std::string &transport_id,  const nlohmann::json& extendedRtpCapabilities,  
		const nlohmann::json& iceParameters,
		const nlohmann::json& iceCandidates,
		const nlohmann::json& dtlsParameters,
		const nlohmann::json& sctpParameters)
	{
		/*const std::string field_trials = "WebRTC-FrameDropper/Disabled/";
		webrtc::field_trial::InitFieldTrialsFromString(field_trials.c_str());*/
		m_networkThread = rtc::Thread::CreateWithSocketServer() ;
		m_signalingThread = rtc::Thread::Create();
		m_workerThread = rtc::Thread::Create();
		m_transport_id = transport_id;

		m_networkThread->SetName("network_thread", nullptr);
		m_signalingThread->SetName("signaling_thread", nullptr);
		m_workerThread->SetName("worker_thread", nullptr);

		if (!m_workerThread->Start() || !m_signalingThread->Start() || !m_networkThread->Start())
		{
			ERROR_EX_LOG(" webrtc network signalig  worker start failed !!!");
			return false;
		}

		m_peer_connection_factory = webrtc::CreatePeerConnectionFactory(
			m_networkThread.get(),
			m_workerThread.get(),
			m_signalingThread.get(),
			nullptr /*default_adm*/,
			webrtc::CreateBuiltinAudioEncoderFactory(),
			webrtc::CreateBuiltinAudioDecoderFactory(),
			// webrtc::CreateBuiltinVideoEncoderFactory() ,
			CreateBuiltinExternalVideoEncoderFactory(),
			webrtc::CreateBuiltinVideoDecoderFactory(),
			nullptr /*audio_mixer*/,
			nullptr /*audio_processing*/);

		webrtc::PeerConnectionInterface::RTCConfiguration config;
		// Set SDP semantics to Unified Plan.
		config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
		m_peer_connection = m_peer_connection_factory->CreatePeerConnection(config, nullptr, nullptr, this);

		nlohmann::json iceParameters_ = iceParameters;
		 
		mediasoupclient::Sdp::RemoteSdp* removesdp_ptr = new mediasoupclient::Sdp::RemoteSdp(iceParameters_, iceCandidates, dtlsParameters, sctpParameters);
		m_remote_sdp.reset( removesdp_ptr );
		m_transport_id = transport_id;
		
		
		return true;
	}
	
	
	bool ctransport::webrtc_connect_transport_setup_connect(const std::string & localDtlsRole)
	{
		//NORMAL_EX_LOG("localDtlsRole = %s", localDtlsRole.c_str());
		//const webrtc::SessionDescriptionInterface* desc = m_peer_connection ->local_description();
		std::string sdp = m_offer;

		//desc->ToString(&sdp);


		nlohmann::json  localSdpObject = sdptransform::parse(sdp);
		// Get our local DTLS parameters.
		nlohmann::json dtlsParameters = mediasoupclient::Sdp::Utils::extractDtlsParameters(localSdpObject);

		// Set our DTLS role.
		dtlsParameters["role"] = localDtlsRole;
		//dtlsParameters["transportId"] = m_transport_id;
		std::string remoteDtlsRole = localDtlsRole == "client" ? "server" : "client";
		 m_remote_sdp->UpdateDtlsRole(remoteDtlsRole);

		// NORMAL_EX_LOG("dtlsParameters = %s", dtlsParameters.dump().c_str());
		// send websocket connectWebRTCTransport --->>>
		 m_client_ptr->_send_connect_webrtc_transport(dtlsParameters);

		return true;
	}


	

	
	
	
	//
	// PeerConnectionObserver implementation.
	//

	 

	void ctransport::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state)
	{
		NORMAL_EX_LOG("OnSignalingChange new_state = %d", new_state);
		if (m_client_ptr && (new_state == webrtc::PeerConnectionInterface::kClosed || new_state == webrtc::PeerConnectionInterface::kHaveRemotePrAnswer))
		{
			m_client_ptr->webrtc_connect_failed_callback();
		}
		else if (!m_client_ptr)
		{
			WARNING_EX_LOG("client ptr = NULL  --->");
		}
		else if (m_client_ptr && new_state == webrtc::PeerConnectionInterface::kHaveRemoteOffer)
		{
			m_client_ptr->set_p2p_connect();
		}
	}

	// 好家伙  webrtc封装太好 ^_^  接口定义 PeerConnectionObserver
	void ctransport::OnAddTrack(
		rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
		const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
		streams)
	{
		NORMAL_EX_LOG("OnAddTrack");
	}
	void ctransport::OnRemoveTrack(
		rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
	{
		NORMAL_EX_LOG("OnRemoveTrack");
	}
	 
	 
	void ctransport::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state)
	{
		NORMAL_EX_LOG("OnIceConnectionChange new_state = %d", new_state);
		if (m_client_ptr && (new_state == webrtc::PeerConnectionInterface::kClosed || new_state == webrtc::PeerConnectionInterface::kHaveRemotePrAnswer))
		{
			m_client_ptr->webrtc_connect_failed_callback();
		}
		else if (m_client_ptr && new_state == webrtc::PeerConnectionInterface::kHaveRemoteOffer)
		{
			m_client_ptr->set_p2p_connect();
		}
	}

	void ctransport::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
	{
		NORMAL_EX_LOG("OnIceCandidate");
	}
	 
 
	// CreateSessionDescriptionObserver implementation.
	void ctransport::OnSuccess(webrtc::SessionDescriptionInterface* desc)
	{
		std::string sdp;

		desc->ToString(&sdp);
		m_offer = sdp;
		//nlohmann::json localsdpobject =  sdptransform::parse(sdp);
		//m_client_ptr->transportofferasner(m_send, true);
	}
	void ctransport::OnFailure(webrtc::RTCError error)
	{
		if (m_client_ptr)
		{
			m_client_ptr->transportofferasner(false, false);
		}
		
	}
}