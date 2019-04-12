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
  const mvcam::GetAdapterReq req;
  mvcam::GetAdapterRes resp;
  grpc::Status status = client.stub->GetAdapter(&context, req, &resp);
  if (!status.ok()) {
    BOOST_TEST_FAIL(status.error_message());
  }
  BOOST_TEST_MESSAGE(resp.adapter().name() << "@" << resp.adapter().version() << ": " << resp.adapter().description());
}

BOOST_AUTO_TEST_CASE(test_list_cameras) {
  client_fixture client;
  grpc::ClientContext context;

  const mvcam::GetDevicesReq req;
  mvcam::GetDevicesRes resp;
  grpc::Status status = client.stub->GetDevices(&context, req, &resp);
  if (!status.ok()) {
    BOOST_TEST_FAIL(status.error_message());
  }

  BOOST_TEST_MESSAGE("Got " << resp.devices_size() << " devices");

  for (int i = 0; i < resp.devices_size(); ++i) {
    BOOST_TEST_MESSAGE(
        "\t#" << i << ": ID=" << resp.devices(i).id()
              << "; Model=" << resp.devices(i).model()
              << "; Manufacture=" << resp.devices(i).manufacture()
              << "; Serial=" << resp.devices(i).serial());
  }
}
BOOST_AUTO_TEST_CASE(test_detect_multiple_times) {
  client_fixture client;
  const mvcam::GetDevicesReq req;
  mvcam::GetDevicesRes resp;

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
  {
    grpc::ClientContext context;
    const mvcam::GetDevicesReq req;
    mvcam::GetDevicesRes resp;

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
    grpc::ClientContext context;
    const mvcam::OpenedReq req;
    mvcam::OpenedRes resp;

    grpc::Status status = client.stub->Opened(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
    BOOST_TEST_CHECK(!resp.opened());
  }

  {
    grpc::ClientContext context;
    mvcam::OpenCameraReq req;
    mvcam::OpenCameraRes resp;
    req.set_id(device.id());
    grpc::Status status = client.stub->OpenCamera(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
  }

  {
    grpc::ClientContext context;
    const mvcam::OpenedReq req;
    mvcam::OpenedRes resp;

    grpc::Status status = client.stub->Opened(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
    BOOST_TEST_CHECK(resp.opened());
  }

  {
    grpc::ClientContext context;

    const mvcam::ShutdownCameraReq req;
    mvcam::ShutdownCameraRes resp;
    grpc::Status status = client.stub->ShutdownCamera(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
  }

  {
    grpc::ClientContext context;
    const mvcam::OpenedReq req;
    mvcam::OpenedRes resp;

    grpc::Status status = client.stub->Opened(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
    BOOST_TEST_CHECK(!resp.opened());
  }
}

BOOST_AUTO_TEST_CASE(test_open_non_existing_camera) {
  client_fixture client;

  {
    grpc::ClientContext context;
    const mvcam::GetDevicesReq req;
    mvcam::GetDevicesRes resp;
    
    grpc::Status status = client.stub->GetDevices(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
  }

  {
    grpc::ClientContext context;
    mvcam::OpenCameraReq req;
    mvcam::OpenCameraRes resp;
    req.set_id("abab123");
    grpc::Status status = client.stub->OpenCamera(&context, req, &resp);
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
  std::string id;

  {
    grpc::ClientContext context;
    const mvcam::GetDevicesReq req;
    mvcam::GetDevicesRes resp;

    grpc::Status status = client.stub->GetDevices(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
    if (resp.devices_size() > 0) {
      id = resp.devices(0).id();
    }
  }
  {
    grpc::ClientContext context;
    mvcam::OpenCameraReq req;
    mvcam::OpenCameraRes resp;
    req.set_id(id);
    grpc::Status status = client.stub->OpenCamera(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
  }

  {
    grpc::ClientContext context;
    const mvcam::GetConfigureReq req;
    mvcam::GetConfigurationRes resp;
    grpc::Status status = client.stub->GetConfiguration(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
#define DISPLAY_PARAM(NAME, V) BOOST_TEST_MESSAGE(NAME <<": " << V.value() << " [" << V.min() << ", " <<V.max()<<"]");
    DISPLAY_PARAM("Exposure", resp.config().exposure());
    DISPLAY_PARAM("Gain", resp.config().gain());
    DISPLAY_PARAM("Frame", resp.config().frame_rate());
  }

  {
    grpc::ClientContext context;
    mvcam::ConfigureCameraReq req;
    mvcam::ConfigureCameraRes resp;

    mvcam::Configuration *const pConfiguration = req.mutable_config();
//    pConfiguration->mutable_frame_number()->set_value(5);
//    pConfiguration->mutable_frame_number()->set_should_update(true);
    pConfiguration->mutable_frame_rate()->set_value(30);
    pConfiguration->mutable_gain()->set_value(5);
    pConfiguration->mutable_exposure()->set_value(45);

    grpc::Status status = client.stub->ConfigureCamera(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
  }

  {
    grpc::ClientContext context;
    const mvcam::GetConfigureReq req;
    mvcam::GetConfigurationRes resp;
    grpc::Status status = client.stub->GetConfiguration(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
#define DISPLAY_PARAM(NAME, V) BOOST_TEST_MESSAGE(NAME <<": " << V.value() << " [" << V.min() << ", " <<V.max()<<"]");
    DISPLAY_PARAM("Exposure", resp.config().exposure());
    DISPLAY_PARAM("Gain", resp.config().gain());
    DISPLAY_PARAM("Frame", resp.config().frame_rate());
  }

  {
    grpc::ClientContext context;

    const mvcam::ShutdownCameraReq req;
    mvcam::ShutdownCameraRes resp;
    grpc::Status status = client.stub->ShutdownCamera(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
  }

}

BOOST_AUTO_TEST_CASE(test_sync_capture) {
  client_fixture client;

  std::string id;
  {
    grpc::ClientContext context;
    const mvcam::GetDevicesReq req;
    mvcam::GetDevicesRes resp;

    grpc::Status status = client.stub->GetDevices(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
    if (resp.devices_size() > 0) {
      id = resp.devices(0).id();
    }
  }

  {
    grpc::ClientContext context;
    mvcam::OpenCameraReq req;
    mvcam::OpenCameraRes resp;
    req.set_id(id);
    grpc::Status status = client.stub->OpenCamera(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
  }

  {
    grpc::ClientContext context;
    mvcam::CaptureReq req;
    mvcam::CaptureRes resp;
    grpc::Status status = client.stub->Capture(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
    BOOST_TEST_MESSAGE("Frame ID" << resp.frame().id());
    BOOST_TEST_MESSAGE("Frame size" << resp.frame().size());
    BOOST_TEST_MESSAGE("Frame height" << resp.frame().height());
    BOOST_TEST_MESSAGE("Frame width" << resp.frame().width());

  }

  {
    grpc::ClientContext context;

    const mvcam::ShutdownCameraReq req;
    mvcam::ShutdownCameraRes resp;
    grpc::Status status = client.stub->ShutdownCamera(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
  }
}

BOOST_AUTO_TEST_CASE(test_async_capture) {
  client_fixture client;
  std::string id;


  {
    grpc::ClientContext context;
    const mvcam::GetDevicesReq req;
    mvcam::GetDevicesRes resp;

    grpc::Status status = client.stub->GetDevices(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
    if (resp.devices_size() > 0) {
      id = resp.devices(0).id();
    }
  }

  {
    grpc::ClientContext context;
    mvcam::OpenCameraReq req;
    mvcam::OpenCameraRes resp;
    req.set_id(id);
    grpc::Status status = client.stub->OpenCamera(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
      return;
    }
  }


  {
    grpc::ClientContext context;

    BOOST_TEST_MESSAGE("Trigger stream for 10 images, batch size = 1");
    mvcam::StreamingReq msg;
    msg.set_batch_size(1);
    msg.set_number_frames(10);
    const std::unique_ptr<grpc::ClientReader<mvcam::FrameStream>> &stream = client.stub->Streaming(&context, msg);

    mvcam::FrameStream fs;

    int frameCount = 0;
    while (stream->Read(&fs)) {
      int size = fs.frames_size();
      frameCount += size;
      BOOST_TEST_MESSAGE("Received frame stream. size = " << size << " count = " << frameCount);
    }
    BOOST_TEST_CHECK(frameCount == 10);
  }

  {
    grpc::ClientContext context;
    mvcam::Frame resp;

    BOOST_TEST_MESSAGE("Trigger stream for 30 images, batch size = 2");
    mvcam::StreamingReq msg;
    msg.set_batch_size(2);
    msg.set_number_frames(30);
    const std::unique_ptr<grpc::ClientReader<mvcam::FrameStream>> &stream = client.stub->Streaming(&context, msg);

    mvcam::FrameStream fs;

    int frameCount = 0;
    while (stream->Read(&fs)) {
      int size = fs.frames_size();
      frameCount += size;
      BOOST_TEST_MESSAGE("Received frame stream. size = " << size << " count = " << frameCount);
      BOOST_TEST_CHECK(size == 2);
    }
    BOOST_TEST_CHECK(frameCount == 30);
  }

  {
    grpc::ClientContext context;
    mvcam::Frame resp;

    BOOST_TEST_MESSAGE("Trigger stream for infinite images, batch size = 1");
    mvcam::StreamingReq msg;
    msg.set_batch_size(1);
    msg.set_number_frames(0);
    const std::unique_ptr<grpc::ClientReader<mvcam::FrameStream>> &stream = client.stub->Streaming(&context, msg);

    mvcam::FrameStream fs;

    int frameCount = 0;
    auto begin = std::chrono::high_resolution_clock::now();
    std::thread cancelThread([&context]() {
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
    if (!status.ok() && status.error_code() != grpc::CANCELLED) {
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

    const mvcam::ShutdownCameraReq req;
    mvcam::ShutdownCameraRes resp;
    grpc::Status status = client.stub->ShutdownCamera(&context, req, &resp);
    if (!status.ok()) {
      BOOST_TEST_FAIL(status.error_message());
    }
  }
}
BOOST_AUTO_TEST_SUITE_END()
