//
// Created by wuyuanyi on 03/01/19.
//

//
// Created by wuyuanyi on 31/12/18.
//

#define BOOST_TEST_MODULE TEST_GRPC_SERVER
#include <boost/test/unit_test.hpp>
#include <thread>
#include <chrono>
#include <future>
#include <grpc++/grpc++.h>
#include "protos/camera_service.grpc.pb.h"
#include "protos/camera_service.pb.h"
#include "server.h"
#include "config.h"
struct adapter_fixture {

  adapter_fixture() : mServer(camera_backend_server::start_server()) {
    // start the server in another process
    BOOST_TEST_MESSAGE("Starting grpc server");

  }
  ~adapter_fixture() {
    BOOST_TEST_MESSAGE("Terminating grpc server");
    mServer->Shutdown();
  }
 private:
  std::unique_ptr<grpc::Server> mServer;
};
BOOST_GLOBAL_FIXTURE(adapter_fixture);

struct client_fixture {
  client_fixture() {
    channel = grpc::CreateChannel(
        std::string("localhost:") + config_provider::get_instance()->read("listen_port"),
        grpc::InsecureChannelCredentials()
    );
    stub = mvcam::MicroVisionCameraService::NewStub(channel);

    BOOST_TEST_MESSAGE("Starting grpc client");
  }
  ~client_fixture() {
  }
  std::shared_ptr<grpc::Channel> channel;
 public:
  std::unique_ptr<mvcam::MicroVisionCameraService::Stub> stub;

};
BOOST_AUTO_TEST_SUITE(test_grpc_camera_server)

BOOST_AUTO_TEST_CASE(test_get_adapter) {
  client_fixture client;
  grpc::ClientContext context;
  google::protobuf::Empty empty;
  mvcam::AvailableAdaptersResponse resp;
  grpc::Status status = client.stub->GetAvailableAdapters(&context, empty, &resp);
  if (!status.ok()) {
    BOOST_TEST_FAIL(status.error_message());
  }

  BOOST_TEST_MESSAGE("Got " << resp.adapters_size() << " mAdapters");

  for (int i = 0; i < resp.adapters_size(); ++i) {
    BOOST_TEST_MESSAGE(
        "\t#" << i << ": " << resp.adapters(i).name() <<
              "@" << resp.adapters(i).version() << ": " << resp.adapters(i).description());
  }
}

BOOST_AUTO_TEST_CASE(test_list_cameras) {
  client_fixture client;
  grpc::ClientContext context;
  mvcam::AdapterRequest req;
  mvcam::DeviceListResponse resp;

  grpc::Status status = client.stub->GetDevices(&context, req, &resp);
  if (!status.ok()) {
    BOOST_TEST_FAIL(status.error_message());
  }

  BOOST_TEST_MESSAGE("Got " << resp.devices_size() << " devices");

  for (int i = 0; i < resp.devices_size(); ++i) {
    BOOST_TEST_MESSAGE(
        "\t#" << i << ": ID=" << resp.devices(i).id()
              << "; Adapter=" << resp.devices(i).adapter().name()
              << "; Model=" << resp.devices(i).model()
              << "; Manufacture=" << resp.devices(i).manufacture()
              << "; Serial=" << resp.devices(i).serial());
  }
}
BOOST_AUTO_TEST_CASE(test_detect_multiple_times) {
  client_fixture client;
  mvcam::AdapterRequest req;
  mvcam::DeviceListResponse resp;

  for (int i = 0; i < 10; ++i) {
    grpc::ClientContext context;
    grpc::Status status = client.stub->GetDevices(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    } else {

      std::this_thread::sleep_for(std::chrono::seconds(1));
      BOOST_TEST_MESSAGE(i);
    }
  }

}
BOOST_AUTO_TEST_CASE(test_open_shutdown_existing_camera) {

  client_fixture client;
  mvcam::DeviceInfo device;
  mvcam::IdRequest idRequest;
  google::protobuf::Empty emptyResponse;
  mvcam::Status statusResponse;

  {
    grpc::ClientContext context;
    mvcam::AdapterRequest req;
    mvcam::DeviceListResponse resp;

    grpc::Status status = client.stub->GetDevices(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
    if (resp.devices_size() == 0) {
      return;
    }
    device = resp.devices(0);
  }

  {
    idRequest.set_id(device.id());

    grpc::ClientContext context;
    grpc::Status status = client.stub->GetStatus(&context, idRequest, &statusResponse);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
    BOOST_TEST_MESSAGE(statusResponse.temperature());
    BOOST_TEST_CHECK(!statusResponse.opened());
    BOOST_TEST_CHECK(!statusResponse.capturing());
  }

  {
    grpc::ClientContext context;
    google::protobuf::Empty emptyResponse;
    grpc::Status status = client.stub->OpenCamera(&context, idRequest, &emptyResponse);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
  }

  {
    grpc::ClientContext context;
    mvcam::Status statusResponse;

    grpc::Status status = client.stub->GetStatus(&context, idRequest, &statusResponse);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
    BOOST_TEST_MESSAGE(statusResponse.temperature());
    BOOST_TEST_CHECK(statusResponse.opened());
    BOOST_TEST_CHECK(!statusResponse.capturing());
  }

  {
    grpc::ClientContext context;

    grpc::Status status = client.stub->ShutdownCamera(&context, idRequest, &emptyResponse);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
  }

  {
    grpc::ClientContext context;

    grpc::Status status = client.stub->GetStatus(&context, idRequest, &statusResponse);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
    BOOST_TEST_MESSAGE(statusResponse.temperature());
    BOOST_TEST_CHECK(!statusResponse.opened());
    BOOST_TEST_CHECK(!statusResponse.capturing());
  }
}

BOOST_AUTO_TEST_CASE(test_open_non_existing_camera) {
  client_fixture client;

  {
    grpc::ClientContext context;
    mvcam::AdapterRequest req;
    mvcam::DeviceListResponse resp;

    grpc::Status status = client.stub->GetDevices(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
  }

  {
    grpc::ClientContext context;
    mvcam::IdRequest idRequest;
    idRequest.set_id("ARBITRARYID");
    google::protobuf::Empty emptyResponse;
    grpc::Status status = client.stub->OpenCamera(&context, idRequest, &emptyResponse);
    if (!status.ok()) {
      BOOST_TEST_MESSAGE(status.error_message());
      BOOST_TEST_CHECK(true);
    } else {
      BOOST_TEST_FAIL("Should not open an non-existing camera without error");
    }
  }
}
BOOST_AUTO_TEST_CASE(test_configure) {
  client_fixture client;
  mvcam::IdRequest idRequest;
  google::protobuf::Empty emptyResponse;

  std::string id;

  {
    grpc::ClientContext context;
    mvcam::AdapterRequest req;
    mvcam::DeviceListResponse resp;

    grpc::Status status = client.stub->GetDevices(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
    if (resp.devices_size() > 0) {
      id = resp.devices(0).id();
      idRequest.set_id(id);
    }
  }
  {
    grpc::ClientContext context;
    google::protobuf::Empty resp;
    grpc::Status status = client.stub->OpenCamera(&context, idRequest, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
  }

  {
    grpc::ClientContext context;
    mvcam::Configuration resp;
    grpc::Status status = client.stub->GetConfiguration(&context, idRequest, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
    BOOST_TEST_MESSAGE("Exposure: " << resp.exposure().value());
    BOOST_TEST_MESSAGE("Gain: " << resp.gain().value());
    BOOST_TEST_MESSAGE("frame rate: " << resp.frame_rate().value());
//    BOOST_TEST_MESSAGE("frame number: " << resp.frame_number().value());
  }

  {
    grpc::ClientContext context;
    mvcam::ConfigureRequest req;

    req.mutable_id()->set_id(id);
    mvcam::Configuration *const pConfiguration = req.mutable_config();
//    pConfiguration->mutable_frame_number()->set_value(5);
//    pConfiguration->mutable_frame_number()->set_should_update(true);
    pConfiguration->mutable_frame_rate()->set_value(30);
    pConfiguration->mutable_frame_rate()->set_should_update(true);
    pConfiguration->mutable_gain()->set_value(5);
    pConfiguration->mutable_gain()->set_should_update(true);
    pConfiguration->mutable_exposure()->set_value(45);
    pConfiguration->mutable_exposure()->set_should_update(true);

    grpc::Status status = client.stub->ConfigureCamera(&context, req, &emptyResponse);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
  }

  {
    grpc::ClientContext context;
    mvcam::Configuration resp;
    grpc::Status status = client.stub->GetConfiguration(&context, idRequest, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
    BOOST_TEST_MESSAGE("Exposure: " << resp.exposure().value());
    BOOST_TEST_MESSAGE("Gain: " << resp.gain().value());
    BOOST_TEST_MESSAGE("frame rate: " << resp.frame_rate().value());
//    BOOST_TEST_MESSAGE("frame number: " << resp.frame_number().value());
  }

  {
    grpc::ClientContext context;
    grpc::Status status = client.stub->ShutdownCamera(&context, idRequest, &emptyResponse);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
  }

}

BOOST_AUTO_TEST_CASE(test_sync_capture) {
  client_fixture client;
  mvcam::IdRequest idRequst;
  google::protobuf::Empty emptyResponse;

  std::string id;
  {
    grpc::ClientContext context;
    mvcam::AdapterRequest req;
    mvcam::DeviceListResponse resp;

    grpc::Status status = client.stub->GetDevices(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
    if (resp.devices_size() > 0) {
      id = resp.devices(0).id();
      idRequst.set_id(id);

    }
  }

  {
    grpc::ClientContext context;
    grpc::Status status = client.stub->OpenCamera(&context, idRequst, &emptyResponse);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
  }

  {
    grpc::ClientContext context;
    mvcam::Frame resp;
    grpc::Status status = client.stub->Capture(&context, idRequst, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
    BOOST_TEST_MESSAGE("Frame ID" << resp.id());
    BOOST_TEST_MESSAGE("Frame size" << resp.size());
    BOOST_TEST_MESSAGE("Frame height" << resp.height());
    BOOST_TEST_MESSAGE("Frame width" << resp.width());

  }

  {
    grpc::ClientContext context;

    grpc::Status status = client.stub->ShutdownCamera(&context, idRequst, &emptyResponse);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
  }
}

BOOST_AUTO_TEST_CASE(test_async_capture) {
  client_fixture client;
  std::string id;
  mvcam::IdRequest idRequest;
  google::protobuf::Empty emptyResponse;

  {
    grpc::ClientContext context;
    mvcam::AdapterRequest req;
    mvcam::DeviceListResponse resp;

    grpc::Status status = client.stub->GetDevices(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
    if (resp.devices_size() > 0) {
      id = resp.devices(0).id();
      idRequest.set_id(id);
    }
  }

  {
    grpc::ClientContext context;
    google::protobuf::Empty resp;
    grpc::Status status = client.stub->OpenCamera(&context, idRequest, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
  }

  {
    grpc::ClientContext context;
    mvcam::Frame resp;


    BOOST_TEST_MESSAGE("Trigger stream for 10 images, batch size = 1");
    mvcam::StreamingRequest msg;
    msg.mutable_id()->set_id(id);
    msg.set_batch_size(1);
    msg.set_number_frames(10);
    const std::unique_ptr<grpc::ClientReader<mvcam::FrameStream>> &stream = client.stub->Streaming(&context, msg);

    mvcam::FrameStream fs;

    int frameCount = 0;
    while (stream->Read(&fs)) {
      int size = fs.frames_size();
      frameCount += size;
      BOOST_TEST_MESSAGE("Received frame stream. size = " <<size << " count = " << frameCount);
    }
    BOOST_TEST_CHECK(frameCount == 10);
  }

  {
    grpc::ClientContext context;
    mvcam::Frame resp;


    BOOST_TEST_MESSAGE("Trigger stream for 30 images, batch size = 2");
    mvcam::StreamingRequest msg;
    msg.mutable_id()->set_id(id);
    msg.set_batch_size(2);
    msg.set_number_frames(30);
    const std::unique_ptr<grpc::ClientReader<mvcam::FrameStream>> &stream = client.stub->Streaming(&context, msg);

    mvcam::FrameStream fs;

    int frameCount = 0;
    while (stream->Read(&fs)) {
      int size = fs.frames_size();
      frameCount += size;
      BOOST_TEST_MESSAGE("Received frame stream. size = " <<size << " count = " << frameCount);
      BOOST_TEST_CHECK(size == 2);
    }
    BOOST_TEST_CHECK(frameCount == 30);
  }

  {
    grpc::ClientContext context;
    mvcam::Frame resp;


    BOOST_TEST_MESSAGE("Trigger stream for infinite images, batch size = 1");
    mvcam::StreamingRequest msg;
    msg.mutable_id()->set_id(id);
    msg.set_batch_size(1);
    msg.set_number_frames(0);
    const std::unique_ptr<grpc::ClientReader<mvcam::FrameStream>> &stream = client.stub->Streaming(&context, msg);

    mvcam::FrameStream fs;

    int frameCount = 0;
    auto begin = std::chrono::high_resolution_clock::now();
    std::thread cancelThread([&context](){
      std::this_thread::sleep_for(std::chrono::seconds(3));
      auto begin = std::chrono::high_resolution_clock::now();
      context.TryCancel();
      BOOST_TEST_MESSAGE("Cancelling the stream");

    });
    while (stream->Read(&fs)) {
      int size = fs.frames_size();
      frameCount += size;
    }
    const grpc::Status &status = stream->Finish();
    if (!status.ok() && status.error_code() != grpc::CANCELLED ) {
      BOOST_TEST_FAIL(status.error_message());
    }

    cancelThread.join();
    auto end = std::chrono::high_resolution_clock::now();
    auto dur = end - begin;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
    BOOST_TEST_MESSAGE("Stopping in " << ms << " with " << frameCount << " images ");
  }
//  std::this_thread::sleep_for(std::chrono::seconds(1)); // shutdown too soon may lead to some error when finalizing?

  {
    grpc::ClientContext context;

    grpc::Status status = client.stub->ShutdownCamera(&context, idRequest, &emptyResponse);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
  }
}
BOOST_AUTO_TEST_SUITE_END()
