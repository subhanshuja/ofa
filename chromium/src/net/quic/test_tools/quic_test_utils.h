// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Common utilities for Quic tests

#ifndef NET_QUIC_TEST_TOOLS_QUIC_TEST_UTILS_H_
#define NET_QUIC_TEST_TOOLS_QUIC_TEST_UTILS_H_

#include <stddef.h>
#include <stdint.h>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/strings/string_piece.h"
#include "net/base/ip_address.h"
#include "net/quic/core/congestion_control/loss_detection_interface.h"
#include "net/quic/core/congestion_control/send_algorithm_interface.h"
#include "net/quic/core/quic_client_push_promise_index.h"
#include "net/quic/core/quic_connection.h"
#include "net/quic/core/quic_framer.h"
#include "net/quic/core/quic_protocol.h"
#include "net/quic/core/quic_sent_packet_manager_interface.h"
#include "net/quic/core/quic_server_session_base.h"
#include "net/quic/core/quic_session.h"
#include "net/quic/core/quic_simple_buffer_allocator.h"
#include "net/quic/test_tools/mock_clock.h"
#include "net/quic/test_tools/mock_random.h"
#include "net/spdy/spdy_framer.h"
#include "net/tools/quic/quic_dispatcher.h"
#include "net/tools/quic/quic_per_connection_packet_writer.h"
#include "net/tools/quic/test_tools/mock_quic_server_session_visitor.h"
#include "testing/gmock/include/gmock/gmock.h"

using base::StringPiece;

// EXPECT_QUIC_BUG is like EXPECT_DFATAL, except it ensures that no DFATAL
// logging is skipped due to exponential backoff.
//
// For external QUIC, EXPECT_QUIC_BUG should be #defined to EXPECT_DFATAL.
#define EXPECT_QUIC_BUG EXPECT_DFATAL

namespace net {

namespace test {

static const QuicConnectionId kTestConnectionId = 42;
static const uint16_t kTestPort = 12345;
static const uint32_t kInitialStreamFlowControlWindowForTest =
    1024 * 1024;  // 1 MB
static const uint32_t kInitialSessionFlowControlWindowForTest =
    1536 * 1024;  // 1.5 MB
// Data stream IDs start at 5: the crypto stream is 1, headers stream is 3.
static const QuicStreamId kClientDataStreamId1 = 5;
static const QuicStreamId kClientDataStreamId2 = 7;
static const QuicStreamId kClientDataStreamId3 = 9;
static const QuicStreamId kServerDataStreamId1 = 4;

// Returns the test peer IP address.
IPAddress TestPeerIPAddress();

// Upper limit on versions we support.
QuicVersion QuicVersionMax();

// Lower limit on versions we support.
QuicVersion QuicVersionMin();

// Returns an address for 127.0.0.1.
IPAddress Loopback4();

// Returns an address for ::1.
IPAddress Loopback6();

// Returns an address for 0.0.0.0.
IPAddress Any4();

void GenerateBody(std::string* body, int length);

// Create an encrypted packet for testing.
// If versions == nullptr, uses &AllSupportedVersions().
// Note that the packet is encrypted with NullEncrypter, so to decrypt the
// constructed packet, the framer must be set to use NullDecrypter.
QuicEncryptedPacket* ConstructEncryptedPacket(
    QuicConnectionId connection_id,
    bool version_flag,
    bool multipath_flag,
    bool reset_flag,
    QuicPathId path_id,
    QuicPacketNumber packet_number,
    const std::string& data,
    QuicConnectionIdLength connection_id_length,
    QuicPacketNumberLength packet_number_length,
    QuicVersionVector* versions,
    Perspective perspective);

// Create an encrypted packet for testing.
// If versions == nullptr, uses &AllSupportedVersions().
// Note that the packet is encrypted with NullEncrypter, so to decrypt the
// constructed packet, the framer must be set to use NullDecrypter.
QuicEncryptedPacket* ConstructEncryptedPacket(
    QuicConnectionId connection_id,
    bool version_flag,
    bool multipath_flag,
    bool reset_flag,
    QuicPathId path_id,
    QuicPacketNumber packet_number,
    const std::string& data,
    QuicConnectionIdLength connection_id_length,
    QuicPacketNumberLength packet_number_length,
    QuicVersionVector* versions);

// This form assumes |versions| == nullptr.
QuicEncryptedPacket* ConstructEncryptedPacket(
    QuicConnectionId connection_id,
    bool version_flag,
    bool multipath_flag,
    bool reset_flag,
    QuicPathId path_id,
    QuicPacketNumber packet_number,
    const std::string& data,
    QuicConnectionIdLength connection_id_length,
    QuicPacketNumberLength packet_number_length);

// This form assumes |connection_id_length| == PACKET_8BYTE_CONNECTION_ID,
// |packet_number_length| == PACKET_6BYTE_PACKET_NUMBER and
// |versions| == nullptr.
QuicEncryptedPacket* ConstructEncryptedPacket(QuicConnectionId connection_id,
                                              bool version_flag,
                                              bool multipath_flag,
                                              bool reset_flag,
                                              QuicPathId path_id,
                                              QuicPacketNumber packet_number,
                                              const std::string& data);

// Constructs a received packet for testing. The caller must take ownership of
// the returned pointer.
QuicReceivedPacket* ConstructReceivedPacket(
    const QuicEncryptedPacket& encrypted_packet,
    QuicTime receipt_time);

// Create an encrypted packet for testing whose data portion erroneous.
// The specific way the data portion is erroneous is not specified, but
// it is an error that QuicFramer detects.
// Note that the packet is encrypted with NullEncrypter, so to decrypt the
// constructed packet, the framer must be set to use NullDecrypter.
QuicEncryptedPacket* ConstructMisFramedEncryptedPacket(
    QuicConnectionId connection_id,
    bool version_flag,
    bool multipath_flag,
    bool reset_flag,
    QuicPathId path_id,
    QuicPacketNumber packet_number,
    const std::string& data,
    QuicConnectionIdLength connection_id_length,
    QuicPacketNumberLength packet_number_length,
    QuicVersionVector* versions,
    Perspective perspective);

void CompareCharArraysWithHexError(const std::string& description,
                                   const char* actual,
                                   const int actual_len,
                                   const char* expected,
                                   const int expected_len);

bool DecodeHexString(const StringPiece& hex, std::string* bytes);

// Returns the length of a QuicPacket that is capable of holding either a
// stream frame or a minimal ack frame.  Sets |*payload_length| to the number
// of bytes of stream data that will fit in such a packet.
size_t GetPacketLengthForOneStream(QuicVersion version,
                                   bool include_version,
                                   bool include_path_id,
                                   bool include_diversification_nonce,
                                   QuicConnectionIdLength connection_id_length,
                                   QuicPacketNumberLength packet_number_length,
                                   size_t* payload_length);

// Returns QuicConfig set to default values.
QuicConfig DefaultQuicConfig();

// Returns a QuicConfig set to default values that supports stateless rejects.
QuicConfig DefaultQuicConfigStatelessRejects();

// Returns a version vector consisting of |version|.
QuicVersionVector SupportedVersions(QuicVersion version);

// Testing convenience method to construct a QuicAckFrame with entropy_hash set
// to 0 and largest_observed from peer set to |largest_observed|.
QuicAckFrame MakeAckFrame(QuicPacketNumber largest_observed);

// Testing convenience method to construct a QuicAckFrame with |num_nack_ranges|
// nack ranges of width 1 packet, starting from |least_unacked|.
QuicAckFrame MakeAckFrameWithNackRanges(size_t num_nack_ranges,
                                        QuicPacketNumber least_unacked);

// Testing convenience method to construct a QuicAckFrame with |num_ack_blocks|
// ack blocks of width 1 packet, starting from |least_unacked| + 2.
QuicAckFrame MakeAckFrameWithAckBlocks(size_t num_ack_blocks,
                                       QuicPacketNumber least_unacked);

// Returns a QuicPacket that is owned by the caller, and
// is populated with the fields in |header| and |frames|, or is nullptr if the
// packet could not be created.
QuicPacket* BuildUnsizedDataPacket(QuicFramer* framer,
                                   const QuicPacketHeader& header,
                                   const QuicFrames& frames);
// Returns a QuicPacket that is owned by the caller, and of size |packet_size|.
QuicPacket* BuildUnsizedDataPacket(QuicFramer* framer,
                                   const QuicPacketHeader& header,
                                   const QuicFrames& frames,
                                   size_t packet_size);

// When constructed, checks that all QUIC flags have their correct default
// values and when destructed, restores those values.
class QuicFlagSaver {
 public:
  QuicFlagSaver();
  ~QuicFlagSaver();
};

// Simple random number generator used to compute random numbers suitable
// for pseudo-randomly dropping packets in tests.  It works by computing
// the sha1 hash of the current seed, and using the first 64 bits as
// the next random number, and the next seed.
class SimpleRandom : public QuicRandom {
 public:
  SimpleRandom() : seed_(0) {}
  ~SimpleRandom() override {}

  // Returns a random number in the range [0, kuint64max].
  uint64_t RandUint64() override;

  void RandBytes(void* data, size_t len) override;
  void Reseed(const void* additional_entropy, size_t len) override;

  void set_seed(uint64_t seed) { seed_ = seed; }

 private:
  uint64_t seed_;

  DISALLOW_COPY_AND_ASSIGN(SimpleRandom);
};

class MockFramerVisitor : public QuicFramerVisitorInterface {
 public:
  MockFramerVisitor();
  ~MockFramerVisitor() override;

  MOCK_METHOD1(OnError, void(QuicFramer* framer));
  // The constructor sets this up to return false by default.
  MOCK_METHOD1(OnProtocolVersionMismatch, bool(QuicVersion version));
  MOCK_METHOD0(OnPacket, void());
  MOCK_METHOD1(OnPublicResetPacket, void(const QuicPublicResetPacket& header));
  MOCK_METHOD1(OnVersionNegotiationPacket,
               void(const QuicVersionNegotiationPacket& packet));
  // The constructor sets this up to return true by default.
  MOCK_METHOD1(OnUnauthenticatedHeader, bool(const QuicPacketHeader& header));
  // The constructor sets this up to return true by default.
  MOCK_METHOD1(OnUnauthenticatedPublicHeader,
               bool(const QuicPacketPublicHeader& header));
  MOCK_METHOD1(OnDecryptedPacket, void(EncryptionLevel level));
  MOCK_METHOD1(OnPacketHeader, bool(const QuicPacketHeader& header));
  MOCK_METHOD1(OnStreamFrame, bool(const QuicStreamFrame& frame));
  MOCK_METHOD1(OnAckFrame, bool(const QuicAckFrame& frame));
  MOCK_METHOD1(OnStopWaitingFrame, bool(const QuicStopWaitingFrame& frame));
  MOCK_METHOD1(OnPaddingFrame, bool(const QuicPaddingFrame& frame));
  MOCK_METHOD1(OnPingFrame, bool(const QuicPingFrame& frame));
  MOCK_METHOD1(OnRstStreamFrame, bool(const QuicRstStreamFrame& frame));
  MOCK_METHOD1(OnConnectionCloseFrame,
               bool(const QuicConnectionCloseFrame& frame));
  MOCK_METHOD1(OnGoAwayFrame, bool(const QuicGoAwayFrame& frame));
  MOCK_METHOD1(OnWindowUpdateFrame, bool(const QuicWindowUpdateFrame& frame));
  MOCK_METHOD1(OnBlockedFrame, bool(const QuicBlockedFrame& frame));
  MOCK_METHOD1(OnPathCloseFrame, bool(const QuicPathCloseFrame& frame));
  MOCK_METHOD0(OnPacketComplete, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockFramerVisitor);
};

class NoOpFramerVisitor : public QuicFramerVisitorInterface {
 public:
  NoOpFramerVisitor() {}

  void OnError(QuicFramer* framer) override {}
  void OnPacket() override {}
  void OnPublicResetPacket(const QuicPublicResetPacket& packet) override {}
  void OnVersionNegotiationPacket(
      const QuicVersionNegotiationPacket& packet) override {}
  bool OnProtocolVersionMismatch(QuicVersion version) override;
  bool OnUnauthenticatedHeader(const QuicPacketHeader& header) override;
  bool OnUnauthenticatedPublicHeader(
      const QuicPacketPublicHeader& header) override;
  void OnDecryptedPacket(EncryptionLevel level) override {}
  bool OnPacketHeader(const QuicPacketHeader& header) override;
  bool OnStreamFrame(const QuicStreamFrame& frame) override;
  bool OnAckFrame(const QuicAckFrame& frame) override;
  bool OnStopWaitingFrame(const QuicStopWaitingFrame& frame) override;
  bool OnPaddingFrame(const QuicPaddingFrame& frame) override;
  bool OnPingFrame(const QuicPingFrame& frame) override;
  bool OnRstStreamFrame(const QuicRstStreamFrame& frame) override;
  bool OnConnectionCloseFrame(const QuicConnectionCloseFrame& frame) override;
  bool OnGoAwayFrame(const QuicGoAwayFrame& frame) override;
  bool OnWindowUpdateFrame(const QuicWindowUpdateFrame& frame) override;
  bool OnBlockedFrame(const QuicBlockedFrame& frame) override;
  bool OnPathCloseFrame(const QuicPathCloseFrame& frame) override;
  void OnPacketComplete() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(NoOpFramerVisitor);
};

class MockQuicConnectionVisitor : public QuicConnectionVisitorInterface {
 public:
  MockQuicConnectionVisitor();
  ~MockQuicConnectionVisitor() override;

  MOCK_METHOD1(OnStreamFrame, void(const QuicStreamFrame& frame));
  MOCK_METHOD1(OnWindowUpdateFrame, void(const QuicWindowUpdateFrame& frame));
  MOCK_METHOD1(OnBlockedFrame, void(const QuicBlockedFrame& frame));
  MOCK_METHOD1(OnRstStream, void(const QuicRstStreamFrame& frame));
  MOCK_METHOD1(OnGoAway, void(const QuicGoAwayFrame& frame));
  MOCK_METHOD3(OnConnectionClosed,
               void(QuicErrorCode error,
                    const std::string& error_details,
                    ConnectionCloseSource source));
  MOCK_METHOD0(OnWriteBlocked, void());
  MOCK_METHOD0(OnCanWrite, void());
  MOCK_METHOD1(OnCongestionWindowChange, void(QuicTime now));
  MOCK_METHOD1(OnConnectionMigration, void(PeerAddressChangeType type));
  MOCK_METHOD0(OnPathDegrading, void());
  MOCK_CONST_METHOD0(WillingAndAbleToWrite, bool());
  MOCK_CONST_METHOD0(HasPendingHandshake, bool());
  MOCK_CONST_METHOD0(HasOpenDynamicStreams, bool());
  MOCK_METHOD1(OnSuccessfulVersionNegotiation,
               void(const QuicVersion& version));
  MOCK_METHOD0(OnConfigNegotiated, void());
  MOCK_METHOD0(PostProcessAfterData, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockQuicConnectionVisitor);
};

class MockQuicConnectionHelper : public QuicConnectionHelperInterface {
 public:
  MockQuicConnectionHelper();
  ~MockQuicConnectionHelper() override;
  const QuicClock* GetClock() const override;
  QuicRandom* GetRandomGenerator() override;
  QuicBufferAllocator* GetBufferAllocator() override;
  void AdvanceTime(QuicTime::Delta delta);

 private:
  MockClock clock_;
  MockRandom random_generator_;
  SimpleBufferAllocator buffer_allocator_;

  DISALLOW_COPY_AND_ASSIGN(MockQuicConnectionHelper);
};

class MockAlarmFactory : public QuicAlarmFactory {
 public:
  QuicAlarm* CreateAlarm(QuicAlarm::Delegate* delegate) override;
  QuicArenaScopedPtr<QuicAlarm> CreateAlarm(
      QuicArenaScopedPtr<QuicAlarm::Delegate> delegate,
      QuicConnectionArena* arena) override;

  // No-op alarm implementation
  class TestAlarm : public QuicAlarm {
   public:
    explicit TestAlarm(QuicArenaScopedPtr<QuicAlarm::Delegate> delegate)
        : QuicAlarm(std::move(delegate)) {}

    void SetImpl() override {}
    void CancelImpl() override {}

    using QuicAlarm::Fire;
  };

  void FireAlarm(QuicAlarm* alarm) {
    reinterpret_cast<TestAlarm*>(alarm)->Fire();
  }
};

class MockQuicConnection : public QuicConnection {
 public:
  // Uses a ConnectionId of 42 and 127.0.0.1:123.
  MockQuicConnection(MockQuicConnectionHelper* helper,
                     MockAlarmFactory* alarm_factory,
                     Perspective perspective);

  // Uses a ConnectionId of 42.
  MockQuicConnection(IPEndPoint address,
                     MockQuicConnectionHelper* helper,
                     MockAlarmFactory* alarm_factory,
                     Perspective perspective);

  // Uses 127.0.0.1:123.
  MockQuicConnection(QuicConnectionId connection_id,
                     MockQuicConnectionHelper* helper,
                     MockAlarmFactory* alarm_factory,
                     Perspective perspective);

  // Uses a ConnectionId of 42, and 127.0.0.1:123.
  MockQuicConnection(MockQuicConnectionHelper* helper,
                     MockAlarmFactory* alarm_factory,
                     Perspective perspective,
                     const QuicVersionVector& supported_versions);

  MockQuicConnection(QuicConnectionId connection_id,
                     IPEndPoint address,
                     MockQuicConnectionHelper* helper,
                     MockAlarmFactory* alarm_factory,
                     Perspective perspective,
                     const QuicVersionVector& supported_versions);

  ~MockQuicConnection() override;

  // If the constructor that uses a MockQuicConnectionHelper has been used then
  // this method
  // will advance the time of the MockClock.
  void AdvanceTime(QuicTime::Delta delta);

  MOCK_METHOD3(ProcessUdpPacket,
               void(const IPEndPoint& self_address,
                    const IPEndPoint& peer_address,
                    const QuicReceivedPacket& packet));
  MOCK_METHOD1(SendConnectionClose, void(QuicErrorCode error));
  MOCK_METHOD3(CloseConnection,
               void(QuicErrorCode error,
                    const std::string& details,
                    ConnectionCloseBehavior connection_close_behavior));
  MOCK_METHOD3(SendConnectionClosePacket,
               void(QuicErrorCode error,
                    const std::string& details,
                    AckBundling ack_mode));
  MOCK_METHOD3(SendRstStream,
               void(QuicStreamId id,
                    QuicRstStreamErrorCode error,
                    QuicStreamOffset bytes_written));
  MOCK_METHOD3(SendGoAway,
               void(QuicErrorCode error,
                    QuicStreamId last_good_stream_id,
                    const std::string& reason));
  MOCK_METHOD1(SendBlocked, void(QuicStreamId id));
  MOCK_METHOD2(SendWindowUpdate,
               void(QuicStreamId id, QuicStreamOffset byte_offset));
  MOCK_METHOD0(OnCanWrite, void());

  MOCK_METHOD1(OnSendConnectionState, void(const CachedNetworkParameters&));
  MOCK_METHOD2(ResumeConnectionState,
               void(const CachedNetworkParameters&, bool));
  MOCK_METHOD1(SetMaxPacingRate, void(QuicBandwidth));

  MOCK_METHOD1(OnError, void(QuicFramer* framer));
  void QuicConnection_OnError(QuicFramer* framer) {
    QuicConnection::OnError(framer);
  }

  void ReallyProcessUdpPacket(const IPEndPoint& self_address,
                              const IPEndPoint& peer_address,
                              const QuicReceivedPacket& packet) {
    QuicConnection::ProcessUdpPacket(self_address, peer_address, packet);
  }

  bool OnProtocolVersionMismatch(QuicVersion version) override { return false; }

  void ReallySendGoAway(QuicErrorCode error,
                        QuicStreamId last_good_stream_id,
                        const std::string& reason) {
    QuicConnection::SendGoAway(error, last_good_stream_id, reason);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockQuicConnection);
};

class PacketSavingConnection : public MockQuicConnection {
 public:
  PacketSavingConnection(MockQuicConnectionHelper* helper,
                         MockAlarmFactory* alarm_factory,
                         Perspective perspective);

  PacketSavingConnection(MockQuicConnectionHelper* helper,
                         MockAlarmFactory* alarm_factory,
                         Perspective perspective,
                         const QuicVersionVector& supported_versions);

  ~PacketSavingConnection() override;

  void SendOrQueuePacket(SerializedPacket* packet) override;

  std::vector<QuicEncryptedPacket*> encrypted_packets_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PacketSavingConnection);
};

class MockQuicSession : public QuicSession {
 public:
  // Takes ownership of |connection|.
  explicit MockQuicSession(QuicConnection* connection);
  ~MockQuicSession() override;

  QuicCryptoStream* GetCryptoStream() override { return crypto_stream_.get(); }

  MOCK_METHOD3(OnConnectionClosed,
               void(QuicErrorCode error,
                    const std::string& error_details,
                    ConnectionCloseSource source));
  MOCK_METHOD1(CreateIncomingDynamicStream,
               ReliableQuicStream*(QuicStreamId id));
  MOCK_METHOD1(CreateOutgoingDynamicStream,
               ReliableQuicStream*(SpdyPriority priority));
  MOCK_METHOD1(ShouldCreateIncomingDynamicStream, bool(QuicStreamId id));
  MOCK_METHOD0(ShouldCreateOutgoingDynamicStream, bool());
  MOCK_METHOD6(WritevData,
               QuicConsumedData(ReliableQuicStream* stream,
                                QuicStreamId id,
                                QuicIOVector data,
                                QuicStreamOffset offset,
                                bool fin,
                                QuicAckListenerInterface*));

  MOCK_METHOD3(SendRstStream,
               void(QuicStreamId stream_id,
                    QuicRstStreamErrorCode error,
                    QuicStreamOffset bytes_written));

  MOCK_METHOD2(OnStreamHeaders,
               void(QuicStreamId stream_id, base::StringPiece headers_data));
  MOCK_METHOD2(OnStreamHeadersPriority,
               void(QuicStreamId stream_id, SpdyPriority priority));
  MOCK_METHOD3(OnStreamHeadersComplete,
               void(QuicStreamId stream_id, bool fin, size_t frame_len));
  MOCK_METHOD0(IsCryptoHandshakeConfirmed, bool());

  using QuicSession::ActivateStream;

  // Returns a QuicConsumedData that indicates all of |data| (and |fin| if set)
  // has been consumed.
  static QuicConsumedData ConsumeAllData(
      ReliableQuicStream* stream,
      QuicStreamId id,
      const QuicIOVector& data,
      QuicStreamOffset offset,
      bool fin,
      QuicAckListenerInterface* ack_notifier_delegate);

 private:
  std::unique_ptr<QuicCryptoStream> crypto_stream_;

  DISALLOW_COPY_AND_ASSIGN(MockQuicSession);
};

class MockQuicSpdySession : public QuicSpdySession {
 public:
  // Takes ownership of |connection|.
  explicit MockQuicSpdySession(QuicConnection* connection);
  ~MockQuicSpdySession() override;

  QuicCryptoStream* GetCryptoStream() override { return crypto_stream_.get(); }
  const SpdyHeaderBlock& GetWriteHeaders() { return write_headers_; }

  // From QuicSession.
  MOCK_METHOD3(OnConnectionClosed,
               void(QuicErrorCode error,
                    const std::string& error_details,
                    ConnectionCloseSource source));
  MOCK_METHOD1(CreateIncomingDynamicStream, QuicSpdyStream*(QuicStreamId id));
  MOCK_METHOD1(CreateOutgoingDynamicStream,
               QuicSpdyStream*(SpdyPriority priority));
  MOCK_METHOD1(ShouldCreateIncomingDynamicStream, bool(QuicStreamId id));
  MOCK_METHOD0(ShouldCreateOutgoingDynamicStream, bool());
  MOCK_METHOD6(WritevData,
               QuicConsumedData(ReliableQuicStream* stream,
                                QuicStreamId id,
                                QuicIOVector data,
                                QuicStreamOffset offset,
                                bool fin,
                                QuicAckListenerInterface*));

  MOCK_METHOD3(SendRstStream,
               void(QuicStreamId stream_id,
                    QuicRstStreamErrorCode error,
                    QuicStreamOffset bytes_written));

  MOCK_METHOD2(OnStreamHeaders,
               void(QuicStreamId stream_id, StringPiece headers_data));
  MOCK_METHOD2(OnStreamHeadersPriority,
               void(QuicStreamId stream_id, SpdyPriority priority));
  MOCK_METHOD3(OnStreamHeadersComplete,
               void(QuicStreamId stream_id, bool fin, size_t frame_len));
  MOCK_METHOD4(OnStreamHeaderList,
               void(QuicStreamId stream_id,
                    bool fin,
                    size_t frame_len,
                    const QuicHeaderList& header_list));
  MOCK_METHOD0(IsCryptoHandshakeConfirmed, bool());
  MOCK_METHOD2(OnPromiseHeaders,
               void(QuicStreamId stream_id, StringPiece headers_data));
  MOCK_METHOD3(OnPromiseHeadersComplete,
               void(QuicStreamId stream_id,
                    QuicStreamId promised_stream_id,
                    size_t frame_len));
  MOCK_METHOD4(OnPromiseHeaderList,
               void(QuicStreamId stream_id,
                    QuicStreamId promised_stream_id,
                    size_t frame_len,
                    const QuicHeaderList& header_list));
  // Methods taking non-copyable types like SpdyHeaderBlock by value cannot be
  // mocked directly.
  size_t WriteHeaders(
      QuicStreamId id,
      SpdyHeaderBlock headers,
      bool fin,
      SpdyPriority priority,
      QuicAckListenerInterface* ack_notifier_delegate) override {
    write_headers_ = std::move(headers);
    return WriteHeadersMock(id, write_headers_, fin, priority,
                            ack_notifier_delegate);
  }
  MOCK_METHOD5(WriteHeadersMock,
               size_t(QuicStreamId id,
                      const SpdyHeaderBlock& headers,
                      bool fin,
                      SpdyPriority priority,
                      QuicAckListenerInterface* ack_notifier_delegate));
  MOCK_METHOD1(OnHeadersHeadOfLineBlocking, void(QuicTime::Delta delta));
  MOCK_METHOD4(
      OnStreamFrameData,
      void(QuicStreamId stream_id, const char* data, size_t len, bool fin));

  using QuicSession::ActivateStream;

 private:
  std::unique_ptr<QuicCryptoStream> crypto_stream_;
  SpdyHeaderBlock write_headers_;

  DISALLOW_COPY_AND_ASSIGN(MockQuicSpdySession);
};

class TestQuicSpdyServerSession : public QuicServerSessionBase {
 public:
  // Takes ownership of |connection|.
  TestQuicSpdyServerSession(QuicConnection* connection,
                            const QuicConfig& config,
                            const QuicCryptoServerConfig* crypto_config,
                            QuicCompressedCertsCache* compressed_certs_cache);
  ~TestQuicSpdyServerSession() override;

  MOCK_METHOD1(CreateIncomingDynamicStream, QuicSpdyStream*(QuicStreamId id));
  MOCK_METHOD1(CreateOutgoingDynamicStream,
               QuicSpdyStream*(SpdyPriority priority));
  QuicCryptoServerStreamBase* CreateQuicCryptoServerStream(
      const QuicCryptoServerConfig* crypto_config,
      QuicCompressedCertsCache* compressed_certs_cache) override;

  QuicCryptoServerStream* GetCryptoStream() override;

  MockQuicCryptoServerStreamHelper* helper() { return &helper_; }

 private:
  MockQuicServerSessionVisitor visitor_;
  MockQuicCryptoServerStreamHelper helper_;

  DISALLOW_COPY_AND_ASSIGN(TestQuicSpdyServerSession);
};

class TestQuicSpdyClientSession : public QuicClientSessionBase {
 public:
  TestQuicSpdyClientSession(QuicConnection* connection,
                            const QuicConfig& config,
                            const QuicServerId& server_id,
                            QuicCryptoClientConfig* crypto_config);
  ~TestQuicSpdyClientSession() override;

  bool IsAuthorized(const std::string& authority) override;

  // QuicClientSessionBase
  MOCK_METHOD1(OnProofValid,
               void(const QuicCryptoClientConfig::CachedState& cached));
  MOCK_METHOD1(OnProofVerifyDetailsAvailable,
               void(const ProofVerifyDetails& verify_details));

  // TestQuicSpdyClientSession
  MOCK_METHOD1(CreateIncomingDynamicStream, QuicSpdyStream*(QuicStreamId id));
  MOCK_METHOD1(CreateOutgoingDynamicStream,
               QuicSpdyStream*(SpdyPriority priority));
  MOCK_METHOD1(ShouldCreateIncomingDynamicStream, bool(QuicStreamId id));
  MOCK_METHOD0(ShouldCreateOutgoingDynamicStream, bool());

  QuicCryptoClientStream* GetCryptoStream() override;

 private:
  std::unique_ptr<QuicCryptoClientStream> crypto_stream_;
  QuicClientPushPromiseIndex push_promise_index_;

  DISALLOW_COPY_AND_ASSIGN(TestQuicSpdyClientSession);
};

class MockPacketWriter : public QuicPacketWriter {
 public:
  MockPacketWriter();
  ~MockPacketWriter() override;

  MOCK_METHOD5(WritePacket,
               WriteResult(const char* buffer,
                           size_t buf_len,
                           const IPAddress& self_address,
                           const IPEndPoint& peer_address,
                           PerPacketOptions* options));
  MOCK_CONST_METHOD0(IsWriteBlockedDataBuffered, bool());
  MOCK_CONST_METHOD0(IsWriteBlocked, bool());
  MOCK_METHOD0(SetWritable, void());
  MOCK_CONST_METHOD1(GetMaxPacketSize,
                     QuicByteCount(const IPEndPoint& peer_address));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockPacketWriter);
};

class MockSendAlgorithm : public SendAlgorithmInterface {
 public:
  MockSendAlgorithm();
  ~MockSendAlgorithm() override;

  MOCK_METHOD2(SetFromConfig,
               void(const QuicConfig& config, Perspective perspective));
  MOCK_METHOD1(SetNumEmulatedConnections, void(int num_connections));
  MOCK_METHOD1(SetMaxCongestionWindow,
               void(QuicByteCount max_congestion_window));
  MOCK_METHOD4(OnCongestionEvent,
               void(bool rtt_updated,
                    QuicByteCount bytes_in_flight,
                    const CongestionVector& acked_packets,
                    const CongestionVector& lost_packets));
  MOCK_METHOD5(OnPacketSent,
               bool(QuicTime,
                    QuicByteCount,
                    QuicPacketNumber,
                    QuicByteCount,
                    HasRetransmittableData));
  MOCK_METHOD1(OnRetransmissionTimeout, void(bool));
  MOCK_METHOD0(OnConnectionMigration, void());
  MOCK_METHOD0(RevertRetransmissionTimeout, void());
  MOCK_CONST_METHOD2(TimeUntilSend,
                     QuicTime::Delta(QuicTime now,
                                     QuicByteCount bytes_in_flight));
  MOCK_CONST_METHOD1(PacingRate, QuicBandwidth(QuicByteCount));
  MOCK_CONST_METHOD0(BandwidthEstimate, QuicBandwidth(void));
  MOCK_CONST_METHOD0(HasReliableBandwidthEstimate, bool());
  MOCK_METHOD1(OnRttUpdated, void(QuicPacketNumber));
  MOCK_CONST_METHOD0(GetCongestionWindow, QuicByteCount());
  MOCK_CONST_METHOD0(GetDebugState, std::string());
  MOCK_CONST_METHOD0(InSlowStart, bool());
  MOCK_CONST_METHOD0(InRecovery, bool());
  MOCK_CONST_METHOD0(GetSlowStartThreshold, QuicByteCount());
  MOCK_CONST_METHOD0(GetCongestionControlType, CongestionControlType());
  MOCK_METHOD2(ResumeConnectionState,
               void(const CachedNetworkParameters&, bool));
  MOCK_METHOD1(OnApplicationLimited, void(QuicByteCount));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockSendAlgorithm);
};

class MockLossAlgorithm : public LossDetectionInterface {
 public:
  MockLossAlgorithm();
  ~MockLossAlgorithm() override;

  MOCK_CONST_METHOD0(GetLossDetectionType, LossDetectionType());
  MOCK_METHOD5(DetectLosses,
               void(const QuicUnackedPacketMap& unacked_packets,
                    QuicTime time,
                    const RttStats& rtt_stats,
                    QuicPacketNumber largest_recently_acked,
                    SendAlgorithmInterface::CongestionVector* packets_lost));
  MOCK_CONST_METHOD0(GetLossTimeout, QuicTime());
  MOCK_METHOD4(SpuriousRetransmitDetected,
               void(const QuicUnackedPacketMap&,
                    QuicTime,
                    const RttStats&,
                    QuicPacketNumber));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockLossAlgorithm);
};

class TestEntropyCalculator
    : public QuicReceivedEntropyHashCalculatorInterface {
 public:
  TestEntropyCalculator();
  ~TestEntropyCalculator() override;

  QuicPacketEntropyHash EntropyHash(
      QuicPacketNumber packet_number) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestEntropyCalculator);
};

class MockEntropyCalculator : public TestEntropyCalculator {
 public:
  MockEntropyCalculator();
  ~MockEntropyCalculator() override;

  MOCK_CONST_METHOD1(EntropyHash,
                     QuicPacketEntropyHash(QuicPacketNumber packet_number));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockEntropyCalculator);
};

class MockAckListener : public QuicAckListenerInterface {
 public:
  MockAckListener();

  MOCK_METHOD2(OnPacketAcked,
               void(int acked_bytes, QuicTime::Delta ack_delay_time));

  MOCK_METHOD1(OnPacketRetransmitted, void(int retransmitted_bytes));

 protected:
  // Object is ref counted.
  ~MockAckListener() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockAckListener);
};

class MockNetworkChangeVisitor
    : public QuicSentPacketManagerInterface::NetworkChangeVisitor {
 public:
  MockNetworkChangeVisitor();
  ~MockNetworkChangeVisitor() override;

  MOCK_METHOD0(OnCongestionChange, void());
  MOCK_METHOD0(OnPathDegrading, void());
  MOCK_METHOD1(OnPathMtuIncreased, void(QuicPacketLength));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockNetworkChangeVisitor);
};

class MockQuicConnectionDebugVisitor : public QuicConnectionDebugVisitor {
 public:
  MockQuicConnectionDebugVisitor();
  ~MockQuicConnectionDebugVisitor() override;

  MOCK_METHOD1(OnFrameAddedToPacket, void(const QuicFrame&));

  MOCK_METHOD5(OnPacketSent,
               void(const SerializedPacket&,
                    QuicPathId,
                    QuicPacketNumber,
                    TransmissionType,
                    QuicTime));

  MOCK_METHOD0(OnPingSent, void());

  MOCK_METHOD3(OnPacketReceived,
               void(const IPEndPoint&,
                    const IPEndPoint&,
                    const QuicEncryptedPacket&));

  MOCK_METHOD1(OnIncorrectConnectionId, void(QuicConnectionId));

  MOCK_METHOD1(OnProtocolVersionMismatch, void(QuicVersion));

  MOCK_METHOD1(OnPacketHeader, void(const QuicPacketHeader& header));

  MOCK_METHOD1(OnSuccessfulVersionNegotiation, void(const QuicVersion&));

  MOCK_METHOD1(OnStreamFrame, void(const QuicStreamFrame&));

  MOCK_METHOD1(OnAckFrame, void(const QuicAckFrame& frame));

  MOCK_METHOD1(OnStopWaitingFrame, void(const QuicStopWaitingFrame&));

  MOCK_METHOD1(OnRstStreamFrame, void(const QuicRstStreamFrame&));

  MOCK_METHOD1(OnConnectionCloseFrame, void(const QuicConnectionCloseFrame&));

  MOCK_METHOD1(OnPublicResetPacket, void(const QuicPublicResetPacket&));

  MOCK_METHOD1(OnVersionNegotiationPacket,
               void(const QuicVersionNegotiationPacket&));
};

class MockReceivedPacketManager : public QuicReceivedPacketManager {
 public:
  explicit MockReceivedPacketManager(QuicConnectionStats* stats);
  ~MockReceivedPacketManager() override;

  MOCK_METHOD2(RecordPacketReceived,
               void(const QuicPacketHeader& header, QuicTime receipt_time));
  MOCK_METHOD1(IsMissing, bool(QuicPacketNumber packet_number));
  MOCK_METHOD1(IsAwaitingPacket, bool(QuicPacketNumber packet_number));
  MOCK_METHOD1(UpdatePacketInformationSentByPeer,
               void(const QuicStopWaitingFrame& stop_waiting));
  MOCK_CONST_METHOD0(HasNewMissingPackets, bool(void));
  MOCK_CONST_METHOD0(ack_frame_updated, bool(void));
};

class MockSentPacketManager : public QuicSentPacketManagerInterface {
 public:
  MockSentPacketManager();
  ~MockSentPacketManager() override;

  MOCK_METHOD1(SetFromConfig, void(const QuicConfig&));
  MOCK_METHOD2(ResumeConnectionState,
               void(const CachedNetworkParameters&, bool));
  MOCK_METHOD1(SetNumOpenStreams, void(size_t));
  MOCK_METHOD1(SetMaxPacingRate, void(QuicBandwidth));
  MOCK_METHOD0(SetHandshakeConfirmed, void(void));
  MOCK_METHOD2(OnIncomingAck, void(const QuicAckFrame&, QuicTime));
  MOCK_METHOD1(RetransmitUnackedPackets, void(TransmissionType));
  MOCK_METHOD0(MaybeRetransmitTailLossProbe, bool(void));
  MOCK_METHOD0(NeuterUnencryptedPackets, void(void));
  MOCK_CONST_METHOD0(HasPendingRetransmissions, bool(void));
  MOCK_METHOD0(NextPendingRetransmission, PendingRetransmission(void));
  MOCK_CONST_METHOD0(HasUnackedPackets, bool(void));
  MOCK_CONST_METHOD1(GetLeastUnacked, QuicPacketNumber(QuicPathId));
  MOCK_METHOD6(OnPacketSent,
               bool(SerializedPacket*,
                    QuicPathId,
                    QuicPacketNumber,
                    QuicTime,
                    TransmissionType,
                    HasRetransmittableData));
  MOCK_METHOD0(OnRetransmissionTimeout, void(void));
  MOCK_METHOD2(TimeUntilSend, QuicTime::Delta(QuicTime, QuicPathId*));
  MOCK_CONST_METHOD0(GetRetransmissionTime, const QuicTime(void));
  MOCK_CONST_METHOD0(GetRttStats, const RttStats*(void));
  MOCK_CONST_METHOD0(BandwidthEstimate, QuicBandwidth(void));
  MOCK_CONST_METHOD0(SustainedBandwidthRecorder,
                     const QuicSustainedBandwidthRecorder*(void));
  MOCK_CONST_METHOD0(GetCongestionWindowInTcpMss, QuicPacketCount(void));
  MOCK_CONST_METHOD1(EstimateMaxPacketsInFlight,
                     QuicPacketCount(QuicByteCount));
  MOCK_CONST_METHOD0(GetCongestionWindowInBytes, QuicByteCount(void));
  MOCK_CONST_METHOD0(GetSlowStartThresholdInTcpMss, QuicPacketCount(void));
  MOCK_CONST_METHOD0(GetDebugState, std::string());
  MOCK_METHOD1(CancelRetransmissionsForStream, void(QuicStreamId));
  MOCK_METHOD2(OnConnectionMigration, void(QuicPathId, PeerAddressChangeType));
  MOCK_CONST_METHOD0(IsHandshakeConfirmed, bool(void));
  MOCK_METHOD1(SetDebugDelegate, void(DebugDelegate*));
  MOCK_CONST_METHOD1(GetLargestObserved, QuicPacketNumber(QuicPathId));
  MOCK_CONST_METHOD1(GetLargestSentPacket, QuicPacketNumber(QuicPathId));
  MOCK_CONST_METHOD1(GetLeastPacketAwaitedByPeer, QuicPacketNumber(QuicPathId));
  MOCK_METHOD1(SetNetworkChangeVisitor, void(NetworkChangeVisitor*));
  MOCK_CONST_METHOD0(InSlowStart, bool(void));
  MOCK_CONST_METHOD0(GetConsecutiveRtoCount, size_t(void));
  MOCK_CONST_METHOD0(GetConsecutiveTlpCount, size_t(void));
  MOCK_METHOD0(OnApplicationLimited, void(void));
};

class MockConnectionCloseDelegate
    : public QuicConnectionCloseDelegateInterface {
 public:
  MockConnectionCloseDelegate();
  ~MockConnectionCloseDelegate() override;

  MOCK_METHOD3(OnUnrecoverableError,
               void(QuicErrorCode,
                    const std::string&,
                    ConnectionCloseSource source));
};

// Creates a client session for testing.
//
// server_id: The server id associated with this stream.
// supports_stateless_rejects:  Does this client support stateless rejects.
// connection_start_time: The time to set for the connection clock.
//   Needed for strike-register nonce verification.  The client
//   connection_start_time should be synchronized witht the server
//   start time, otherwise nonce verification will fail.
// supported_versions: Set of QUIC versions this client supports.
// helper: Pointer to the MockQuicConnectionHelper to use for the session.
// crypto_client_config: Pointer to the crypto client config.
// client_connection: Pointer reference for newly created
//   connection.  This object will be owned by the
//   client_session.
// client_session: Pointer reference for the newly created client
//   session.  The new object will be owned by the caller.
void CreateClientSessionForTest(QuicServerId server_id,
                                bool supports_stateless_rejects,
                                QuicTime::Delta connection_start_time,
                                QuicVersionVector supported_versions,
                                MockQuicConnectionHelper* helper,
                                MockAlarmFactory* alarm_factory,
                                QuicCryptoClientConfig* crypto_client_config,
                                PacketSavingConnection** client_connection,
                                TestQuicSpdyClientSession** client_session);

// Creates a server session for testing.
//
// server_id: The server id associated with this stream.
// connection_start_time: The time to set for the connection clock.
//   Needed for strike-register nonce verification.  The server
//   connection_start_time should be synchronized witht the client
//   start time, otherwise nonce verification will fail.
// supported_versions: Set of QUIC versions this server supports.
// helper: Pointer to the MockQuicConnectionHelper to use for the session.
// crypto_server_config: Pointer to the crypto server config.
// server_connection: Pointer reference for newly created
//   connection.  This object will be owned by the
//   server_session.
// server_session: Pointer reference for the newly created server
//   session.  The new object will be owned by the caller.
void CreateServerSessionForTest(
    QuicServerId server_id,
    QuicTime::Delta connection_start_time,
    QuicVersionVector supported_versions,
    MockQuicConnectionHelper* helper,
    MockAlarmFactory* alarm_factory,
    QuicCryptoServerConfig* crypto_server_config,
    QuicCompressedCertsCache* compressed_certs_cache,
    PacketSavingConnection** server_connection,
    TestQuicSpdyServerSession** server_session);

// Helper to generate client side stream ids, generalizes
// kClientDataStreamId1 etc. above.
QuicStreamId QuicClientDataStreamId(int i);

// Verifies that the relative error of |actual| with respect to |expected| is
// no more than |margin|.

template <typename T>
void ExpectApproxEq(T expected, T actual, float relative_margin) {
  // If |relative_margin| > 1 and T is an unsigned type, the comparison will
  // underflow.
  ASSERT_LE(relative_margin, 1);
  ASSERT_GE(relative_margin, 0);

  T absolute_margin = expected * relative_margin;

  EXPECT_GE(expected + absolute_margin, actual);
  EXPECT_LE(expected - absolute_margin, actual);
}

}  // namespace test
}  // namespace net

#endif  // NET_QUIC_TEST_TOOLS_QUIC_TEST_UTILS_H_