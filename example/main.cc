#include <iostream>
#include <thread>
#include <camera/camera.h>
#include <camera/photography_settings.h>
#include <camera/device_discovery.h>
#include <regex>
#include <vector>
#include <string>

//**************** Image stiching ****************
#include <stitcher/stitcher.h>
#include <stitcher/common.h>
#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <chrono>

using namespace std::chrono;


class TestStreamDelegate : public ins_camera::StreamDelegate {
public:
	TestStreamDelegate() {
		file1_ = fopen("./01.h264", "wb");
		file2_ = fopen("./02.h264", "wb");
	}
	~TestStreamDelegate() {
		fclose(file1_);
		fclose(file2_);
	}

	void OnAudioData(const uint8_t* data, size_t size, int64_t timestamp) override {
		std::cout << "on audio data:" << std::endl;
	}
	void OnVideoData(const uint8_t* data, size_t size, int64_t timestamp, uint8_t streamType, int stream_index = 0) override {
		//std::cout << "on video frame:" << size << ";" << timestamp << std::endl;
		if (stream_index == 0) {
			fwrite(data, sizeof(uint8_t), size, file1_);
		}
		if (stream_index == 1) {
			fwrite(data, sizeof(uint8_t), size, file2_);
		}
	}
	void OnGyroData(const std::vector<ins_camera::GyroData>& data) override {
		//for (auto& gyro : data) {
		//	if (gyro.timestamp - last_timestamp > 2) {
		//		fprintf(file1_, "timestamp:%lld package_size = %d  offtimestamp = %lld gyro:[%f %f %f] accel:[%f %f %f]\n", gyro.timestamp, data.size(), gyro.timestamp - last_timestamp, gyro.gx, gyro.gy, gyro.gz, gyro.ax, gyro.ay, gyro.az);
		//	}
		//	last_timestamp = gyro.timestamp;
		//}
	}
	void OnExposureData(const ins_camera::ExposureData& data) override {
		//fprintf(file2_, "timestamp:%lld shutter_speed_s:%f\n", data.timestamp, data.exposure_time);
	}

private:
	FILE* file1_;
	FILE* file2_;
	int64_t last_timestamp = 0;
};

int main(int argc, char* argv[]) {
	std::cout << "Begin open camera..." << std::endl;
	ins_camera::DeviceDiscovery discovery;
	auto list = discovery.GetAvailableDevices();
	for (int i = 0; i < list.size(); ++i) {
		auto desc = list[i];
		std::cout << "\nSerial:" << desc.serial_number << "\t"
			<< "Camera type:" << int(desc.camera_type) << "\t"
			<< "Lens type:" << int(desc.lens_type) << "\n" << std::endl;
	}

	if (list.size() <= 0) {
		std::cerr << "No device found." << std::endl;
		return -1;
	}

	std::shared_ptr<ins_camera::Camera> cam = std::make_shared<ins_camera::Camera>(list[0].info);
	//ins_camera::Camera cam(list[0].info);
	if (!cam->Open()) {
		std::cerr << "Failed to open camera" << std::endl;
		return -1;
	}

	std::cout << "\nhttp base url:" << cam->GetHttpBaseUrl() << std::endl;

	std::shared_ptr<ins_camera::StreamDelegate> delegate = std::make_shared<TestStreamDelegate>();
	cam->SetStreamDelegate(delegate);

	discovery.FreeDeviceDescriptors(list);

	std::cout << "Succeed to open camera!\n" << std::endl;

	std::cout << "Usage:\n" << std::endl;
	std::cout << "1: Take photo" << std::endl;
	std::cout << "2: Get file list(video and photo)" << std::endl;
	std::cout << "3: Delete file" << std::endl;
	std::cout << "4: Download file" << std::endl;
	std::cout << "5: Test set exposure settings" << std::endl;
	std::cout << "6: Test set capture settings" << std::endl;
	std::cout << "7: Test take photo and download" << std::endl;
	std::cout << "8: Get current capture status" << std::endl;
	std::cout << "9: Start timelapse" << std::endl;
	std::cout << "10: Stop timelapse" << std::endl;
	std::cout << "11: Get battery status" << std::endl;
	std::cout << "12: Get storage info" << std::endl;
	std::cout << "13: Stitch image" << std::endl;
	std::cout << "0: Exit\n" << std::endl;

	auto camera_type = cam->GetCameraType();

	auto start = time(NULL);
	cam->SyncLocalTimeToCamera(start);

	int option;
	while (true) {
		std::cout << "Please enter index: ";
		std::cin >> option;
		if (option < 0 || option > 30) {
			std::cout << "Invalid index" << std::endl;
			continue;
		}

		if (option == 0) {
			break;
		}

		//Take photo
		if (option == 1) {
			const auto url = cam->TakePhoto();
			if (!url.IsSingleOrigin() || url.Empty()) {
				std::cout << "Failed to take picture" << std::endl;
				break;
			}
			std::cout << "Take picture done: " << url.GetSingleOrigin() << std::endl;
		}

		//Get camera file list(video and photo)
		if (option == 2) {
			const auto file_list = cam->GetCameraFilesList();
			for (const auto& file : file_list) {
				std::cout << "File: " << file << std::endl;
			}
		}

		//Delete file from camera
		if (option == 3) {
			std::string file_to_delete;
			std::cout << "Please input full file path to delete: ";
			std::cin >> file_to_delete;
			const auto ret = cam->DeleteCameraFile(file_to_delete);
			if (ret) {
				std::cout << "Deletion succeed" << std::endl;
			}
		}

		//Download file from camera
		if (option == 4) {
			std::string path_to_download = "/DCIM/Camera01/";
			std::string image_to_download;

			std::string path_to_save = "C:/Users/signa/Desktop/MasterThesis/Images/";
			std::string image_to_save;

			std::cout << "Please input image to download: ";
			std::cin >> image_to_download;
			std::cout << "Please input image to save: ";
			std::cin >> image_to_save;

			std::string full_path_download = path_to_download + image_to_download;
			std::string full_path_save = path_to_save + image_to_save;

			const auto ret = cam->DownloadCameraFile(full_path_download,
				full_path_save);
			if (ret) {
				std::cout << "Download " << image_to_download << " succeed!!!" << std::endl;
			}
			else {
				std::cout << "Download " << image_to_download << " failed!!!" << std::endl;
			}
		}

		//Test set exposure settings
		if (option == 5) {
			auto exposure_settings = cam->GetExposureSettings(ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE);
			if (exposure_settings) {
				std::cout << "EVBias : " << exposure_settings->EVBias() << std::endl;
				std::cout << "ISO    : " << exposure_settings->Iso() << std::endl;
				std::cout << "speed  : " << exposure_settings->ShutterSpeed() << std::endl;
			}

			int bias;
			std::cout << "Please enter EVBIOS: ";
			std::cin >> bias;
			auto exposure = std::make_shared<ins_camera::ExposureSettings>();
			auto exposure_mode = ins_camera::PhotographyOptions_ExposureMode::PhotographyOptions_ExposureOptions_Program_AUTO;
			if (camera_type == ins_camera::CameraType::Insta360X3) {
				exposure_mode = ins_camera::PhotographyOptions_ExposureMode::PhotographyOptions_ExposureOptions_Program_FULL_AUTO;
			}
			exposure->SetExposureMode(exposure_mode);
			exposure->SetEVBias(bias); // range -80 ~ 80, default 0, step 1
			exposure->SetIso(800);
			exposure->SetShutterSpeed(1.0 / 120.0);
			auto ret = cam->SetExposureSettings(ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE, exposure);
			if (ret) {
				auto exposure_settings = cam->GetExposureSettings(ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE);
				std::cout << "Success! ISO " << exposure_settings->Iso() << ", WB = " << exposure_settings->ShutterSpeed() << ", ExposureMode = " << exposure_settings->ExposureMode() << std::endl;
			}
			else {
				std::cerr << "Failed to set exposure" << std::endl;
			}
		}

		//Test set capture settings
		if (option == 6) {
			auto settings = std::make_shared<ins_camera::CaptureSettings>();
			settings->SetValue(ins_camera::CaptureSettings::CaptureSettings_Saturation, 0);
			settings->SetWhiteBalance(ins_camera::PhotographyOptions_WhiteBalance_WB_4000K);
			settings->SetValue(ins_camera::CaptureSettings::CaptureSettings_Brightness, 100);
			auto ret = cam->SetCaptureSettings(ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE, settings);
			if (ret) {
				auto capture_settings = cam->GetCaptureSettings(ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE);
				std::cout << "Success!" << std::endl;
			}
			else {
				std::cerr << "Failed to set capture settings" << std::endl;
			}
		}

		//Test take photoand download
		if (option == 7) {
			int test_cout = 100;
			int test = -1;
			while (test_cout-- != 0) {
				const auto url = cam->TakePhoto();
				if (!url.IsSingleOrigin() || url.Empty()) {
					std::cout << "Failed to take picture" << std::endl;
					return -1;
				}

				std::string download_url = url.GetSingleOrigin();
				std::string image_insp = download_url.substr(download_url.rfind("/") + 1);
				std::string image_jpg = image_insp.substr(0, image_insp.find('.')) + ".jpg";
				std::string save_path = "C:/Users/signa/Desktop/MasterThesis/Images/" + image_jpg;

				const auto ret = cam->DownloadCameraFile(download_url, save_path);
				if (ret) {
					std::cout << "Download " << image_jpg << " succeed!!!" << std::endl;
				}
				else {
					std::cout << "Download " << image_jpg << " failed!!!" << std::endl;
				}
			}
			cam->Close();
			return 0;
		}

		//Get current capture status
		if (option == 8) {
			auto ret = cam->GetCaptureCurrentStatus();
			if (ret == ins_camera::CaptureStatus::NOT_CAPTURE) {
				std::cout << "Current statue : not capture" << std::endl;;
			}
			else {
				std::cout << "Current statue : capture" << std::endl;
			}
		}

		//Start timelapse
		if (option == 9) {
			ins_camera::TimelapseParam param;
			param.mode = ins_camera::CameraTimelapseMode::MOBILE_TIMELAPSE_VIDEO;
			param.duration = -1;
			param.lapseTime = 3000;
			param.accelerate_fequency = 5;
			if (!cam->SetTimeLapseOption(param)) {
				std::cerr << "Failed to set capture settings." << std::endl;
			}
			else {
				auto ret = cam->StartTimeLapse(param.mode);
				if (ret) {
					std::cerr << "Success!" << std::endl;
				}
				else {
					std::cerr << "Failed to start timelapse" << std::endl;
				}
			}
		}

		//Stop timelapse
		if (option == 10) {
			auto url = cam->StopTimeLapse(ins_camera::CameraTimelapseMode::MOBILE_TIMELAPSE_VIDEO);
			if (url.Empty()) {
				std::cerr << "Stop timelapse failed" << std::endl;
				continue;
			}
			auto& origins = url.OriginUrls();
			std::cout << "Stop timelapse success" << std::endl;
			for (auto& origin_url : origins) {
				std::cout << "Url:" << origin_url << std::endl;
			}
		}

		//Get battery status
		if (option == 11) {
			if (!cam->IsConnected()) {
				std::cout << "Device is offline" << std::endl;
				break;
			}

			ins_camera::BatteryStatus status;
			bool ret = cam->GetBatteryStatus(status);
			if (!ret) {
				std::cerr << "GetBatteryStatus failed" << std::endl;
				continue;
			}
			std::cout << "PowerType : " << status.power_type << std::endl;
			std::cout << "Battery_level : " << status.battery_level << std::endl;
			std::cout << "Battery_scale : " << status.battery_scale << std::endl;
		}

		//Get storage info
		if (option == 12) {
			ins_camera::StorageStatus status;
			bool ret = cam->GetStorageState(status);
			if (!ret) {
				std::cerr << "GetBatteryStatus failed" << std::endl;
				continue;
			}
			std::cout << "Free_space : " << status.free_space << std::endl;
			std::cout << "Total_space : " << status.total_space << std::endl;
			std::cout << "State : " << status.state << std::endl;
		}

		if (option == 13) {

			std::vector<std::string> input_paths;
			std::string input_image;
			std::string output_image;
			std::string output_path;

			std::cout << "Please input image to stitch:";

			while (std::cin >> input_image) {

				std::string path = "C:/Users/Victoria/Documents/Github/MasterThesis/Images/";
				std::string full_path = path + input_image;
				input_paths.push_back(full_path);

				if (std::cin.get() == '\n')
					std::cout << "Please name the stitched image: ";

				std::cin >> output_image;
				output_path = path + output_image + ".jpg";
				break;
			}
			// Stichtypes: 
			//TEMPLATE (fast-not good), OPTFLOW (slow-best), DYNAMICSTICH (fast-relative good)
			STITCH_TYPE stitch_type = STITCH_TYPE::TEMPLATE;
			HDR_TYPE hdr_type = HDR_TYPE::ImageHdr_NONE;

			int output_width = 1920;
			int output_height = 960;

			bool enable_flowstate = true;
			bool enable_cuda = false;
			bool enalbe_stitchfusion = true;
			bool enable_colorplus = true;
			bool enable_directionlock = true;
			bool enable_denoise = true;
			std::string colorpuls_model_path;


			if (input_paths.empty()) {
				std::cout << "Can not find input_file" << std::endl;
				return -1;
			}

			if (output_path.empty()) {
				std::cout << "Can not find output_file" << std::endl;
				return -1;
			}

			if (colorpuls_model_path.empty()) {
				enable_colorplus = false;
			}


			int count = 1;
			while (count--) {
				std::string suffix = input_paths[0].substr(input_paths[0].find_last_of(".") + 1);
				std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
				if (suffix == "insp" || suffix == "jpg") {
					auto image_stitcher = std::make_shared<ins_media::ImageStitcher>();
					image_stitcher->SetInputPath(input_paths);
					image_stitcher->SetStitchType(stitch_type);
					image_stitcher->SetHDRType(hdr_type);
					image_stitcher->SetOutputPath(output_path);
					image_stitcher->SetOutputSize(output_width, output_height);
					image_stitcher->EnableFlowState(enable_flowstate);
					image_stitcher->EnableDenoise(enable_denoise);
					image_stitcher->EnableCuda(enable_cuda);
					image_stitcher->EnableColorPlus(enable_colorplus, colorpuls_model_path);
					image_stitcher->Stitch();
				}
				std::cout << "Stitching succeded! \n";
			}
		}

		if (option == 30) {
			const auto file_list = cam->GetCameraFilesList();
			for (const auto& file : file_list) {
				std::string save_path = "D:/testImage" + file;
				int ret = cam->DownloadCameraFile(file, save_path);
				if (ret) {
					std::cout << "Download " << file << " succeed!!!" << std::endl;
				}
				else {
					std::cout << "Download " << file << " failed!!!" << std::endl;
				}
			}
		}

		if (option == 31) {
			const auto file_list = cam->GetCameraFilesList();
			for (const auto& file : file_list) {
				const auto ret = cam->DeleteCameraFile(file);
				if (ret) {
					std::cout << file << " Deletion succeed" << std::endl;
				}
			}
		}
	}
	cam->Close();
	return 0;
}