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
#include "cdevice.h"
#include <api/peer_connection_interface.h> // webrtc::PeerConnectionInterface
#include <future>                          // std::promise, std::future
#include <memory>                          // std::unique_ptr
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/create_peerconnection_factory.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <rtc_base/ssl_adapter.h>
#include "external_video_encoder_factory.h"
#include "csession_description.h"
#include "clog.h"
#include "Utils.hpp"
#include "sdp/Utils.hpp"
namespace chen {
	constexpr uint16_t SctpNumStreamsOs{ 1024u };
	constexpr uint16_t SctpNumStreamsMis{ 1024u };

	nlohmann:: json SctpNumStreams = { { "OS", SctpNumStreamsOs }, { "MIS", SctpNumStreamsMis } };


	class PrivateListener : public webrtc::PeerConnectionObserver
	{
		/* Virtual methods inherited from PeerConnectionObserver. */
	public:
		void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState newState) override;
		void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
		void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
		void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> dataChannel) override;
		void OnRenegotiationNeeded() override;
		void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState newState) override;
		void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState newState) override;
		void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
		void OnIceCandidatesRemoved(const std::vector<cricket::Candidate>& candidates) override;
		void OnIceConnectionReceivingChange(bool receiving) override;
		void OnAddTrack(
			rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
			const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) override;
		void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;
		void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
		void OnInterestingUsage(int usagePattern) override;
	};
	/* PeerConnection::PrivateListener */

	/**
	* Triggered when the SignalingState changed.
	*/
	void PrivateListener::OnSignalingChange(
		webrtc::PeerConnectionInterface::SignalingState newState)
	{
		

		//MSC_DEBUG("[newState:%s]", PeerConnection::signalingState2String[newState].c_str());
	}

	/**
	* Triggered when media is received on a new stream from remote peer.
	*/
	void PrivateListener::OnAddStream(
		rtc::scoped_refptr<webrtc::MediaStreamInterface> /*stream*/)
	{
	
	}

	/**
	* Triggered when a remote peer closes a stream.
	*/
	void PrivateListener::OnRemoveStream(
		rtc::scoped_refptr<webrtc::MediaStreamInterface> /*stream*/)
	{
		
	}

	/**
	* Triggered when a remote peer opens a data channel.
	*/
	void PrivateListener::OnDataChannel(
		rtc::scoped_refptr<webrtc::DataChannelInterface> /*dataChannel*/)
	{
	
	}

	/**
	* Triggered when renegotiation is needed. For example, an ICE restart has begun.
	*/
	void PrivateListener::OnRenegotiationNeeded()
	{
		
	}

	/**
	* Triggered any time the IceConnectionState changes.
	*
	* Note that our ICE states lag behind the standard slightly. The most
	* notable differences include the fact that "failed" occurs after 15
	* seconds, not 30, and this actually represents a combination ICE + DTLS
	* state, so it may be "failed" if DTLS fails while ICE succeeds.
	*/
	void PrivateListener::OnIceConnectionChange(
		webrtc::PeerConnectionInterface::IceConnectionState newState)
	{
		//MSC_TRACE();

	//	MSC_DEBUG("[newState:%s]", iceConnectionState2String[newState].c_str());
	}

	/**
	* Triggered any time the IceGatheringState changes.
	*/
	void PrivateListener::OnIceGatheringChange(
		webrtc::PeerConnectionInterface::IceGatheringState newState)
	{
		///MSC_TRACE();

		//MSC_DEBUG("[newState:%s]", PeerConnection::iceGatheringState2String[newState].c_str());
	}

	/**
	* Triggered when a new ICE candidate has been gathered.
	*/
	void PrivateListener::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
	{
		

		std::string candidateStr;

		candidate->ToString(&candidateStr);

		//MSC_DEBUG("[candidate:%s]", candidateStr.c_str());
	}

	/**
	* Triggered when the ICE candidates have been removed.
	*/
	void PrivateListener::OnIceCandidatesRemoved(
		const std::vector<cricket::Candidate>& /*candidates*/)
	{
		 
	}

	/**
	* Triggered when the ICE connection receiving status changes.
	*/
	void  PrivateListener::OnIceConnectionReceivingChange(bool /*receiving*/)
	{
	 
	}

	/**
	* Triggered when a receiver and its track are created.
	*
	* Note: This is called with both Plan B and Unified Plan semantics. Unified
	* Plan users should prefer OnTrack, OnAddTrack is only called as backwards
	* compatibility (and is called in the exact same situations as OnTrack).
	*/
	void  PrivateListener::OnAddTrack(
		rtc::scoped_refptr<webrtc::RtpReceiverInterface> /*receiver*/,
		const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& /*streams*/)
	{
	 
	}

	/**
	* Triggered when signaling indicates a transceiver will be receiving
	*
	* media from the remote endpoint. This is fired during a call to
	* SetRemoteDescription. The receiving track can be accessed by:
	* transceiver->receiver()->track() and its associated streams by
	* transceiver->receiver()->streams().
	*
	* NOTE: This will only be called if Unified Plan semantics are specified.
	* This behavior is specified in section 2.2.8.2.5 of the "Set the
	* RTCSessionDescription" algorithm:
	*   https://w3c.github.io/webrtc-pc/#set-description
	*/
	void  PrivateListener::OnTrack(
		rtc::scoped_refptr<webrtc::RtpTransceiverInterface> /*transceiver*/)
	{
		 
	}

	/**
	* Triggered when signaling indicates that media will no longer be received on a
	* track.
	*
	* With Plan B semantics, the given receiver will have been removed from the
	* PeerConnection and the track muted.
	* With Unified Plan semantics, the receiver will remain but the transceiver
	* will have changed direction to either sendonly or inactive.
	*   https://w3c.github.io/webrtc-pc/#process-remote-track-removal
	*/
	void  PrivateListener::OnRemoveTrack(
		rtc::scoped_refptr<webrtc::RtpReceiverInterface> /*receiver*/)
	{
	 
	}

	/**
	* Triggered when an interesting usage is detected by WebRTC.
	*
	* An appropriate action is to add information about the context of the
	* PeerConnection and write the event to some kind of "interesting events"
	* log function.
	* The heuristics for defining what constitutes "interesting" are
	* implementation-defined.
	*/
	void  PrivateListener::OnInterestingUsage(int /*usagePattern*/)
	{ 
	}

	class cpeerconnection
	{
	public:
		cpeerconnection(PrivateListener* privateListener)
		{
			webrtc::PeerConnectionInterface::RTCConfiguration config;
			// Set SDP semantics to Unified Plan.
			config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
			this->networkThread = rtc::Thread::CreateWithSocketServer();
			this->signalingThread = rtc::Thread::Create();
			this->workerThread = rtc::Thread::Create();

			this->networkThread->SetName("network_thread", nullptr);
			this->signalingThread->SetName("signaling_thread", nullptr);
			this->workerThread->SetName("worker_thread", nullptr);

			if (!this->networkThread->Start() || !this->signalingThread->Start() || !this->workerThread->Start())
			{
				ERROR_EX_LOG("thread start errored");
				return;
			}

			this->peerConnectionFactory = webrtc::CreatePeerConnectionFactory(
				this->networkThread.get(),
				this->workerThread.get(),
				this->signalingThread.get(),
				nullptr /*default_adm*/,
				 webrtc::CreateBuiltinAudioEncoderFactory() ,
				webrtc::CreateBuiltinAudioDecoderFactory(),
				// webrtc::CreateBuiltinVideoEncoderFactory(),
				CreateBuiltinExternalVideoEncoderFactory()  ,
				webrtc::CreateBuiltinVideoDecoderFactory(),
				nullptr /*audio_mixer*/,
				nullptr /*audio_processing*/);


			printf("[%s][%d] config.sdp_semantics = %d\n",  __FUNCTION__, __LINE__, config.sdp_semantics);

			// Create the webrtc::Peerconnection.
			 pc = peerConnectionFactory->CreatePeerConnection(config, nullptr, nullptr, privateListener);
			 printf("[%s][%d] config.sdp_semantics = %d\n",  __FUNCTION__, __LINE__, config.sdp_semantics);
		}
		~cpeerconnection()
		{
			peerConnectionFactory = nullptr;
			pc = nullptr;
		}
		rtc::scoped_refptr<webrtc::RtpTransceiverInterface>  AddTransceiver(
			cricket::MediaType mediaType)
		{
			auto result = this->pc->AddTransceiver(mediaType);

			if (!result.ok())
			{
				rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver = nullptr;

				return transceiver;
			}

			return result.value();
		}
		std::string CreateOffer()
		{
			
			CreateSessionDescriptionObserver* sessionDescriptionObserver =
				new rtc::RefCountedObject<CreateSessionDescriptionObserver>();

			auto future = sessionDescriptionObserver->GetFuture();
			webrtc::PeerConnectionInterface::RTCOfferAnswerOptions config;
			 pc->CreateOffer(sessionDescriptionObserver, config);

			return future.get();
		}

		void Destroy()
		{
			if (peerConnectionFactory)
			{
				peerConnectionFactory->StopAecDump();
			}

			peerConnectionFactory = nullptr;
			pc = nullptr;
			if (networkThread)
			{
				networkThread->Stop();
			}
			if (signalingThread)
			{
				signalingThread->Stop();
			}
			if (workerThread)
			{
				workerThread->Stop();
			}
		}
	private:
		// Signaling and worker threads.
		std::unique_ptr<rtc::Thread> networkThread;
		std::unique_ptr<rtc::Thread> signalingThread;
		std::unique_ptr<rtc::Thread> workerThread;

		// PeerConnection factory.
		rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peerConnectionFactory;

		// PeerConnection instance.
		rtc::scoped_refptr<webrtc::PeerConnectionInterface> pc;

	};
	



	
	namespace deivce
	{
		nlohmann::json GetNativeSctpCapabilities()
		{


			json caps = { { "numStreams", SctpNumStreams } };

			return caps;
		}
		 bool GetNativeRtpCapabilities(nlohmann::json & nativeRtpCapabilities)
		{
			  std::unique_ptr< PrivateListener> privateListener(
				  new  PrivateListener());
			  if (!privateListener)
			  {
				  WARNING_EX_LOG("alloc failed !!!");
				  return false;
			  }
			 //std::shared_ptr<PrivateListener> privateListener_ptr = std::make_shared<PrivateListener>();
			std::unique_ptr<cpeerconnection> pc(
				new cpeerconnection(privateListener.get()));
			if (!pc)
			{
				WARNING_EX_LOG("alloc failed !!!");
				return false;
			}
			/*if (!pc->AddTransceiver(cricket::MediaType::MEDIA_TYPE_AUDIO))
			{
				WARNING_EX_LOG("AddTransceiver  audio failed  ");
				return false;
			}*/
			if (!pc->AddTransceiver(cricket::MediaType::MEDIA_TYPE_VIDEO))
			{
				WARNING_EX_LOG("AddTransceiver  video  failed ");
				return false;
			}

			webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;

			// May throw.
			std::string  offer = pc->CreateOffer();
			nlohmann::json sdpObject  ;
			try
			{
				sdpObject =  sdptransform::parse(offer);
			}  
			catch (...)
			{
				ERROR_EX_LOG("json CreateOffer parse  failed  [offer = %s] !!!", offer.c_str());
				pc->Destroy();
				return false;
			}
			
			 
			try 
			{
				  nativeRtpCapabilities = mediasoupclient::Sdp::Utils::extractRtpCapabilities(sdpObject);
				 
			}
			catch (...)
			{
				ERROR_EX_LOG(" json extractRtpCapabilities parse failed  [offer = %s] !!!", offer.c_str());
				pc->Destroy();
				return false;
			}
			pc->Destroy();
			return true;
		}
		//bool load(nlohmann::json routerRtpCapabilities)
		//{
		//	// Get Native RTP capabilities.
		//	nlohmann::json native_RtpCapabilities = GetNativeRtpCapabilities();

		//	return true;
		//}
	}

}