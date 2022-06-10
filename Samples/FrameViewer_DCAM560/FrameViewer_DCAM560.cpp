//Note:DCAM550 has no RGB frame and no use DCAM_305 in Vzense_dcamtype.h

#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include "DCAM560/Vzense_api_560.h"
#include <thread>
#include <windows.system.h>

#define FPS
#define FPS_LEN 100
using namespace std;
using namespace cv;

#ifdef FPS

SYSTEMTIME sys;
long delayT = 0;

int countof_loop_tof = 0;
long tatoldelay_tof = 0;
int fps_tof = 0;

int countof_loop_rgb = 0;
long tatoldelay_rgb = 0;
int fps_rgb = 0;

int countof_loop_ir = 0;
long tatoldelay_ir = 0;
int fps_ir = 0;

int countof_loop_wdr1 = 0;
long tatoldelay_wdr1 = 0;
int fps_wdr1 = 0;


int countof_loop_wdr2 = 0;
long tatoldelay_wdr2 = 0;
int fps_wdr2 = 0;

int countof_loop_wdr3 = 0;
long tatoldelay_wdr3 = 0;
int fps_wdr3 = 0;

#endif
void HotPlugStateCallback(const PsDeviceInfo* pInfo, int params);

static void Opencv_Depth(uint32_t slope, int height, int width, uint8_t*pData, cv::Mat& dispImg)
{
	dispImg = cv::Mat(height, width, CV_16UC1, pData);
	Point2d pointxy(width / 2, height / 2);
	int val = dispImg.at<ushort>(pointxy);
	char text[20];
	sprintf_s(text, "%d", val);
	dispImg.convertTo(dispImg, CV_8U, 255.0 / slope);
	applyColorMap(dispImg, dispImg, cv::COLORMAP_RAINBOW);
	int color;
	if (val > 2500)
		color = 0;
	else
		color = 4096;
	circle(dispImg, pointxy, 4, Scalar(color, color, color), -1, 8, 0);
	putText(dispImg, text, pointxy, FONT_HERSHEY_DUPLEX, 2, Scalar(color, color, color));
}

int main(int argc, char *argv[])
{
	PsReturnStatus status;
	uint32_t deviceIndex = 0;
	uint32_t deviceCount = 0;
	uint32_t slope = 1450;
	uint32_t wdrSlope = 4400;
	uint32_t wdrRange1Slope = 1450;
	uint32_t wdrRange2Slope = 4400;
	uint32_t wdrRange3Slope = 6000;

	PsDepthRange depthRange = PsNearRange;
	PsDataMode dataMode = PsDepthAndIRAndRGB_30;
	PsWDROutputMode wdrMode = { PsWDRTotalRange_Two, PsNearRange, 1, PsFarRange, 1, PsUnknown, 1 };
	bool f_bWDRMode = false;
	bool bWDRStyle = true;
	bool f_bConfidence = false;

	status = Ps2_Initialize();
	if (status != PsReturnStatus::PsRetOK)
	{
		cout << "PsInitialize failed!" << endl;
		system("pause");
		return -1;
	}

GET:
	status = Ps2_GetDeviceCount(&deviceCount);
	if (status != PsReturnStatus::PsRetOK)
	{
		cout << "PsGetDeviceCount failed! make sure the DCAM is connected" << endl;
		this_thread::sleep_for(chrono::seconds(1));
		goto GET;
	}
	cout << "Get device count: " << deviceCount << endl;
	if (0 == deviceCount)
	{
		this_thread::sleep_for(chrono::seconds(1));
		goto GET;
	}
	Ps2_SetHotPlugStatusCallback(HotPlugStateCallback);

	PsDeviceInfo* pDeviceListInfo = new PsDeviceInfo[deviceCount];
	status = Ps2_GetDeviceListInfo(pDeviceListInfo, deviceCount);
	PsDeviceHandle deviceHandle = 0;
	status = Ps2_OpenDevice(pDeviceListInfo->uri, &deviceHandle);
	if (status != PsReturnStatus::PsRetOK)
	{
		cout << "OpenDevice failed!" << endl;
		system("pause");
		return -1;
	}
	uint32_t sessionIndex = 0;

	status = Ps2_StartStream(deviceHandle, sessionIndex);
	if (status != PsReturnStatus::PsRetOK)
	{
		cout << "StartStream failed!" << endl;
		system("pause");
		return -1;
	}

	PsCameraParameters cameraParameters;
	status = Ps2_GetCameraParameters(deviceHandle, sessionIndex, PsDepthSensor, &cameraParameters);

	cout << "Get PsGetCameraParameters status: " << status << endl;
	cout << "Depth Camera Intinsic: " << endl;
	cout << "Fx: " << cameraParameters.fx << endl;
	cout << "Cx: " << cameraParameters.cx << endl;
	cout << "Fy: " << cameraParameters.fy << endl;
	cout << "Cy: " << cameraParameters.cy << endl;
	cout << "Depth Distortion Coefficient: " << endl;
	cout << "K1: " << cameraParameters.k1 << endl;
	cout << "K2: " << cameraParameters.k2 << endl;
	cout << "P1: " << cameraParameters.p1 << endl;
	cout << "P2: " << cameraParameters.p2 << endl;
	cout << "K3: " << cameraParameters.k3 << endl;
	cout << "K4: " << cameraParameters.k4 << endl;
	cout << "K5: " << cameraParameters.k5 << endl;
	cout << "K6: " << cameraParameters.k6 << endl;

	//Get MeasuringRange
	PsMeasuringRange measuringrange = { 0 };

	status = Ps2_GetDataMode(deviceHandle, sessionIndex, &dataMode);
	if (status != PsReturnStatus::PsRetOK)
		cout << "Ps2_GetDataMode failed!" << endl;
	else
		cout << "Get Ps2_GetDataMode : " << dataMode << endl;
	if (dataMode == PsWDR_Depth)
	{
		f_bWDRMode = true;

		status = Ps2_GetWDROutputMode(deviceHandle, sessionIndex, &wdrMode);

		if (status != PsReturnStatus::PsRetOK)
			cout << "Ps2_GetWDROutputMode failed!" << endl;
		else
		{
			if (wdrMode.totalRange == PsWDRTotalRange_Two)
			{
				depthRange = wdrMode.range2;
				status = Ps2_GetMeasuringRange(deviceHandle, sessionIndex, (PsDepthRange)wdrMode.range1, &measuringrange);
				if (status != PsReturnStatus::PsRetOK)
					cout << "Ps2_GetMeasuringRange failed!" << endl;
				else
				{
					switch ((PsDepthRange)wdrMode.range1)
					{
					case PsNearRange:
					case PsXNearRange:
					case PsXXNearRange:
						wdrRange1Slope = measuringrange.effectDepthMaxNear;
						break;

					case PsMidRange:
					case PsXMidRange:
					case PsXXMidRange:
						wdrRange1Slope = measuringrange.effectDepthMaxMid;
						break;

					case PsFarRange:
					case PsXFarRange:
					case PsXXFarRange:

						wdrRange1Slope = measuringrange.effectDepthMaxFar;
						break;
					default:
						break;
					}
					cout << "wdrRange1Slope   =  " << wdrRange1Slope << endl;
				}

				status = Ps2_GetMeasuringRange(deviceHandle, sessionIndex, (PsDepthRange)wdrMode.range2, &measuringrange);
				if (status != PsReturnStatus::PsRetOK)
					cout << "Ps2_GetMeasuringRange failed!" << endl;
				else
				{
					switch ((PsDepthRange)wdrMode.range2)
					{
					case PsNearRange:
					case PsXNearRange:
					case PsXXNearRange:
						wdrRange2Slope = wdrSlope == measuringrange.effectDepthMaxNear;
						break;

					case PsMidRange:
					case PsXMidRange:
					case PsXXMidRange:
						wdrRange2Slope = wdrSlope = measuringrange.effectDepthMaxMid;
						break;

					case PsFarRange:
					case PsXFarRange:
					case PsXXFarRange:

						wdrRange2Slope = wdrSlope = measuringrange.effectDepthMaxFar;
						break;
					default:
						break;
					}
					cout << "wdrSlope   =  wdrRange2Slope  " << wdrSlope << endl;
				}

			}
			else if (wdrMode.totalRange == PsWDRTotalRange_Three)
			{
				status = Ps2_GetMeasuringRange(deviceHandle, sessionIndex, (PsDepthRange)wdrMode.range1, &measuringrange);
				if (status != PsReturnStatus::PsRetOK)
					cout << "Ps2_GetMeasuringRange failed!" << endl;
				else
				{
					switch ((PsDepthRange)wdrMode.range1)
					{
					case PsNearRange:
					case PsXNearRange:
					case PsXXNearRange:
						wdrRange1Slope = measuringrange.effectDepthMaxNear;
						break;

					case PsMidRange:
					case PsXMidRange:
					case PsXXMidRange:
						wdrRange1Slope = measuringrange.effectDepthMaxMid;
						break;

					case PsFarRange:
					case PsXFarRange:
					case PsXXFarRange:

						wdrRange1Slope = measuringrange.effectDepthMaxFar;
						break;
					default:
						break;
					}
					cout << "wdrRange1Slope   =  " << wdrRange1Slope << endl;
				}


				status = Ps2_GetMeasuringRange(deviceHandle, sessionIndex, (PsDepthRange)wdrMode.range2, &measuringrange);
				if (status != PsReturnStatus::PsRetOK)
					cout << "Ps2_GetMeasuringRange failed!" << endl;
				else
				{
					switch ((PsDepthRange)wdrMode.range2)
					{
					case PsNearRange:
					case PsXNearRange:
					case PsXXNearRange:
						wdrRange2Slope = measuringrange.effectDepthMaxNear;
						break;

					case PsMidRange:
					case PsXMidRange:
					case PsXXMidRange:
						wdrRange2Slope = measuringrange.effectDepthMaxMid;
						break;

					case PsFarRange:
					case PsXFarRange:
					case PsXXFarRange:

						wdrRange2Slope = measuringrange.effectDepthMaxFar;
						break;
					default:
						break;
					}
					cout << "wdrRange2Slope   =  " << wdrSlope << endl;
				}
				status = Ps2_GetMeasuringRange(deviceHandle, sessionIndex, (PsDepthRange)wdrMode.range3, &measuringrange);
				if (status != PsReturnStatus::PsRetOK)
					cout << "Ps2_GetMeasuringRange failed!" << endl;
				else
				{
					switch ((PsDepthRange)wdrMode.range3)
					{
					case PsNearRange:
					case PsXNearRange:
					case PsXXNearRange:
						wdrRange3Slope = wdrSlope = measuringrange.effectDepthMaxNear;
						break;

					case PsMidRange:
					case PsXMidRange:
					case PsXXMidRange:
						wdrRange3Slope = wdrSlope = measuringrange.effectDepthMaxMid;
						break;

					case PsFarRange:
					case PsXFarRange:
					case PsXXFarRange:

						wdrRange3Slope = wdrSlope = measuringrange.effectDepthMaxFar;
						break;
					default:
						break;
					}
					cout << "wdrSlope   =  wdrRange3Slope  " << wdrSlope << endl;
				}
			}
		}
	}
	else
	{
		status = Ps2_GetDepthRange(deviceHandle, sessionIndex, &depthRange);
		if (status != PsReturnStatus::PsRetOK)
			cout << "Ps2_GetDepthRange failed!" << endl;
		else
			cout << "Get Depth Range " << depthRange << endl;

		status = Ps2_GetMeasuringRange(deviceHandle, sessionIndex, depthRange, &measuringrange);
		if (status != PsReturnStatus::PsRetOK)
			cout << "Ps2_GetMeasuringRange failed!" << endl;
		else
		{
			switch (depthRange)
			{
			case PsNearRange:
			case PsXNearRange:
			case PsXXNearRange:
				slope = measuringrange.effectDepthMaxNear;
				break;

			case PsMidRange:
			case PsXMidRange:
			case PsXXMidRange:
				slope = measuringrange.effectDepthMaxMid;
				break;

			case PsFarRange:
			case PsXFarRange:
			case PsXXFarRange:

				slope = measuringrange.effectDepthMaxFar;
				break;
			default:
				break;
			}
			cout << "slope  ==  " << slope << endl;
		}
	}
	cv::Mat imageMat;
	const string irImageWindow = "IR Image";
	const string depthImageWindow = "Depth Image";
	const string wdrDepthImageWindow = "WDR Depth Image";
	const string wdrDepthRange1ImageWindow = "WDR Depth Range1 Image";
	const string wdrDepthRange2ImageWindow = "WDR Depth Range2 Image";
	const string wdrDepthRange3ImageWindow = "WDR Depth Range3 Image";

	ofstream PointCloudWriter;
	PsDepthVector3 DepthVector = { 0, 0, 0 };
	PsVector3f WorldVector = { 0.0f };

	bool f_bPointClound = false;

	PsDepthRangeList rangelist = { 0 };
	int len = sizeof(rangelist);
	status = Ps2_GetProperty(deviceHandle, sessionIndex, PsPropertyDepthRangeList, &rangelist, &len);

	if (status == PsReturnStatus::PsRetOK&&rangelist.count > 0)
	{
		cout << "Available Range List: ";
		for (int i = 0; i < rangelist.count - 1; i++)
		{
			cout << (int)rangelist.depthrangelist[i] << ",";
		}
		cout << (int)rangelist.depthrangelist[rangelist.count - 1] << endl;
	}

	//Set PixelFormat as PsPixelFormatBGR888 for opencv display
	Ps2_SetColorPixelFormat(deviceHandle, sessionIndex, PsPixelFormatBGR888);

	status = Ps2_GetCameraParameters(deviceHandle, sessionIndex, PsRgbSensor, &cameraParameters);

	cout << "Get PsGetCameraParameters status: " << status << endl;
	cout << "RGB Camera Intinsic: " << endl;
	cout << "Fx: " << cameraParameters.fx << endl;
	cout << "Cx: " << cameraParameters.cx << endl;
	cout << "Fy: " << cameraParameters.fy << endl;
	cout << "Cy: " << cameraParameters.cy << endl;
	cout << "RGB Distortion Coefficient: " << endl;
	cout << "K1: " << cameraParameters.k1 << endl;
	cout << "K2: " << cameraParameters.k2 << endl;
	cout << "K3: " << cameraParameters.k3 << endl;
	cout << "P1: " << cameraParameters.p1 << endl;
	cout << "P2: " << cameraParameters.p2 << endl;

	PsCameraExtrinsicParameters CameraExtrinsicParameters;
	status = Ps2_GetCameraExtrinsicParameters(deviceHandle, sessionIndex, &CameraExtrinsicParameters);

	cout << "Get PsGetCameraExtrinsicParameters status: " << status << endl;
	cout << "Camera rotation: " << endl;
	cout << CameraExtrinsicParameters.rotation[0] << " "
		<< CameraExtrinsicParameters.rotation[1] << " "
		<< CameraExtrinsicParameters.rotation[2] << " "
		<< CameraExtrinsicParameters.rotation[3] << " "
		<< CameraExtrinsicParameters.rotation[4] << " "
		<< CameraExtrinsicParameters.rotation[5] << " "
		<< CameraExtrinsicParameters.rotation[6] << " "
		<< CameraExtrinsicParameters.rotation[7] << " "
		<< CameraExtrinsicParameters.rotation[8] << " "
		<< endl;

	cout << "Camera transfer: " << endl;
	cout << CameraExtrinsicParameters.translation[0] << " "
		<< CameraExtrinsicParameters.translation[1] << " "
		<< CameraExtrinsicParameters.translation[2] << " " << endl;

	const string rgbImageWindow = "RGB Image";
	const string mappedDepthImageWindow = "MappedDepth Image";
	const string mappedRgbImageWindow = "MappedRGB Image";
	bool f_bMappedRGB = true;
	bool f_bMappedDepth = true;
	bool f_bSync = false;

	cout << "\n--------------------------------------------------------------------" << endl;
	cout << "--------------------------------------------------------------------" << endl;
	cout << "Press following key to set corresponding feature:" << endl;
	cout << "0/1/2...: Change depth range Near/Middle/Far..." << endl;
	cout << "P/p: Save point cloud data into PointCloud.txt in current directory" << endl;
	cout << "T/t: Change background filter threshold value" << endl;
	cout << "S/s: Enable or disable the Depth and RGB synchronize feature " << endl;
	cout << "M/m: Change data mode: input corresponding index in terminal:" << endl;
	cout << "                    0: Output Depth and RGB in 30 fps" << endl;
	cout << "                    1: Output IR and RGB in 30 fps" << endl;
	cout << "                    2: Output Depth and IR and RGB in 30 fps" << endl;
	cout << "                    3: Output WDR_Depth and RGB in 30 fps" << endl;
	cout << "R/r: Change the RGB resolution: input corresponding index in terminal:" << endl;
	cout << "                             0: 1600*1200" << endl;
	cout << "                             1: 800*600" << endl;
	cout << "                             2: 640*480" << endl;
	cout << "Q/q: Enable or disable the mapped RGB in Depth space" << endl;
	cout << "L/l: Enable or disable the mapped Depth in RGB space" << endl;
	cout << "V/v: Enable or disable the WDR depth fusion feature" << endl;
	cout << "C/c: Enable or disable the ConfidenceFilter" << endl;
	cout << "F/f: Set the ConfidenceFilter Threshold" << endl;
	cout << "H/h: Set the WDRConfidenceFilter Threshold in DataMode(PsWDR_Depth)" << endl;
	cout << "B/b: Enable or disable the RGB manual exposure" << endl;
	cout << "N/n: Set the RGB absolute exposure, in [1,4000] and the unit is 100us" << endl;
	cout << "Esc: Program quit " << endl;
	cout << "--------------------------------------------------------------------" << endl;
	cout << "--------------------------------------------------------------------\n" << endl;

	int destroycount = 0;

	for (;;)
	{
		if (destroycount > 0)
		{
			destroycount--;
			cv::destroyAllWindows();
		}
		PsFrame depthFrame = { 0 };
		PsFrame irFrame = { 0 };
		PsFrame wdrDepthFrame = { 0 };
		PsFrame rgbFrame = { 0 };
		PsFrame mappedDepthFrame = { 0 };
		PsFrame mappedRGBFrame = { 0 };

		// Read one frame before call PsGetFrame
		PsFrameReady frameReady = { 0 };
		status = Ps2_ReadNextFrame(deviceHandle, sessionIndex, &frameReady);

#ifdef FPS

		GetLocalTime(&sys);
		long dwEnd = sys.wMilliseconds;

		long timedelay = dwEnd - delayT;
		delayT = dwEnd;
		if (timedelay < 0) {
			timedelay += 1000;
		}
		tatoldelay_tof += timedelay;
		tatoldelay_rgb += timedelay;
		tatoldelay_ir += timedelay;
		tatoldelay_wdr1 += timedelay;
		tatoldelay_wdr2 += timedelay;
		tatoldelay_wdr3 += timedelay;

#endif
		if (status != PsRetOK)
		{
			goto KEY;
		}

		//Get depth frame, depth frame only output in following data mode
		if (1 == frameReady.depth)
		{
			status = Ps2_GetFrame(deviceHandle, sessionIndex, PsDepthFrame, &depthFrame);

			if (depthFrame.pFrameData != NULL)
			{

				if (f_bPointClound)
				{
					PointCloudWriter.open("PointCloud.txt");
					PsFrame &srcFrame = depthFrame;
					const int len = srcFrame.width * srcFrame.height;
					PsVector3f* worldV = new PsVector3f[len];

					Ps2_ConvertDepthFrameToWorldVector(deviceHandle, sessionIndex, srcFrame, worldV); //Convert Depth frame to World vectors.

					for (int i = 0; i < len; i++)
					{
						if (worldV[i].z == 0 || worldV[i].z == 0xFFFF)
							continue; //discard zero points
						PointCloudWriter << worldV[i].x << "\t" << worldV[i].y << "\t" << worldV[i].z << std::endl;
					}
					delete[] worldV;
					worldV = NULL;
					std::cout << "Save point cloud successful in PointCloud.txt" << std::endl;
					PointCloudWriter.close();
					f_bPointClound = false;
				}
				//Display the Depth Image
				if (f_bWDRMode&&dataMode == PsWDR_Depth)
				{
					if (depthFrame.depthRange == wdrMode.range1&&wdrMode.range1Count != 0)
					{
#ifdef FPS
						countof_loop_wdr1++;
						if (countof_loop_wdr1 >= FPS_LEN)
						{
							fps_wdr1 = 1000 * FPS_LEN / tatoldelay_wdr1;
							//cout << fps_tof<<endl;
							countof_loop_wdr1 = 0;
							tatoldelay_wdr1 = 0;
						}
#endif
						Opencv_Depth(wdrRange1Slope, depthFrame.height, depthFrame.width, depthFrame.pFrameData, imageMat);
#ifdef FPS
						if (fps_wdr1 != 0)
						{
							char fps[20];
							sprintf_s(fps, "FPS: %d", fps_wdr1);
							putText(imageMat, fps, Point2d(10, 20), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0, 0, 0));
						}
#endif
						cv::imshow(wdrDepthRange1ImageWindow, imageMat);
					}
					else if (depthFrame.depthRange == wdrMode.range2&&wdrMode.range2Count != 0)
					{
#ifdef FPS
						countof_loop_wdr2++;
						if (countof_loop_wdr2 >= FPS_LEN)
						{
							fps_wdr2 = 1000 * FPS_LEN / tatoldelay_wdr2;
							//cout << fps_tof<<endl;
							countof_loop_wdr2 = 0;
							tatoldelay_wdr2 = 0;
						}
#endif
						Opencv_Depth(wdrRange2Slope, depthFrame.height, depthFrame.width, depthFrame.pFrameData, imageMat);
#ifdef FPS
						if (fps_wdr2 != 0)
						{
							char fps[20];
							sprintf_s(fps, "FPS: %d", fps_wdr2);
							putText(imageMat, fps, Point2d(10, 20), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0, 0, 0));
						}
#endif
						cv::imshow(wdrDepthRange2ImageWindow, imageMat);
					}
					else if (depthFrame.depthRange == wdrMode.range3&&wdrMode.range3Count != 0)
					{
#ifdef FPS
						countof_loop_wdr3++;
						if (countof_loop_wdr3 >= FPS_LEN)
						{
							fps_wdr3 = 1000 * FPS_LEN / tatoldelay_wdr3;
							//cout << fps_tof<<endl;
							countof_loop_wdr3 = 0;
							tatoldelay_wdr3 = 0;
						}
#endif
						Opencv_Depth(wdrRange3Slope, depthFrame.height, depthFrame.width, depthFrame.pFrameData, imageMat);
#ifdef FPS
						if (fps_wdr3 != 0)
						{
							char fps[20];
							sprintf_s(fps, "FPS: %d", fps_wdr3);
							putText(imageMat, fps, Point2d(10, 20), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0, 0, 0));
						}
#endif
						cv::imshow(wdrDepthRange3ImageWindow, imageMat);
					}
				}
				else
				{
#ifdef FPS
					countof_loop_tof++;
					if (countof_loop_tof >= FPS_LEN)
					{
						fps_tof = 1000 * FPS_LEN / tatoldelay_tof;
						//cout << fps_tof<<endl;
						countof_loop_tof = 0;
						tatoldelay_tof = 0;
					}
#endif
					Opencv_Depth(slope, depthFrame.height, depthFrame.width, depthFrame.pFrameData, imageMat);
#ifdef FPS
					if (fps_tof != 0)
					{
						char fps[20];
						sprintf_s(fps, "FPS: %d", fps_tof);
						putText(imageMat, fps, Point2d(10, 20), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0, 0, 0));
					}
#endif
					cv::imshow(depthImageWindow, imageMat);
				}
			}
			else
			{
				cout << "Ps2_GetFrame PsDepthFrame status:" << status << " pFrameData is NULL " << endl;
			}
		}

		//Get IR frame, IR frame only output in following data mode
		if (1 == frameReady.ir)
		{
			status = Ps2_GetFrame(deviceHandle, sessionIndex, PsIRFrame, &irFrame);

			if (irFrame.pFrameData != NULL)
			{
#ifdef FPS	 
				countof_loop_ir++;
				if (countof_loop_ir >= FPS_LEN)
				{
					fps_ir = 1000 * FPS_LEN / tatoldelay_ir;
					//cout << fps_tof << endl;
					countof_loop_ir = 0;
					tatoldelay_ir = 0;
				}
#endif
				//Display the IR Image
				imageMat = cv::Mat(irFrame.height, irFrame.width, CV_16UC1, irFrame.pFrameData);

				// Convert 16bit IR pixel (max pixel value is 3840) to 8bit for display
				imageMat.convertTo(imageMat, CV_8U, 255.0 / 3840);
#ifdef FPS
				if (fps_ir != 0)
				{
					char fps[20];
					sprintf_s(fps, "FPS: %d", fps_ir);
					putText(imageMat, fps, Point2d(10, 20), FONT_HERSHEY_DUPLEX, 0.5, Scalar(255, 255, 255));
				}
#endif
				cv::imshow(irImageWindow, imageMat);
			}
			else
			{
				cout << "Ps2_GetFrame PsIRFrame status:" << status << " pFrameData is NULL " << endl;
			}
		}

		//Get WDR depth frame(fusion or alternatively, determined by Ps2_SetWDRStyle, default in fusion)
		//WDR depth frame only output in PsWDR_Depth data mode
		if (1 == frameReady.wdrDepth)
		{
			status = Ps2_GetFrame(deviceHandle, sessionIndex, PsWDRDepthFrame, &wdrDepthFrame);
			if (wdrDepthFrame.pFrameData != NULL)
			{
#ifdef FPS			 
				countof_loop_tof++;
				if (countof_loop_tof >= FPS_LEN)
				{
					fps_tof = 1000 * FPS_LEN / tatoldelay_tof;
					//cout << fps_tof << endl;
					countof_loop_tof = 0;
					tatoldelay_tof = 0;
				}
#endif
				if (f_bPointClound)
				{
					PointCloudWriter.open("PointCloud.txt");
					PsFrame &srcFrame = wdrDepthFrame;
					const int len = srcFrame.width * srcFrame.height;
					PsVector3f* worldV = new PsVector3f[len];

					Ps2_ConvertDepthFrameToWorldVector(deviceHandle, sessionIndex, srcFrame, worldV); //Convert Depth frame to World vectors.

					for (int i = 0; i < len; i++)
					{
						if (worldV[i].z == 0 || worldV[i].z == 0xFFFF)
							continue; //discard zero points
						PointCloudWriter << worldV[i].x << "\t" << worldV[i].y << "\t" << worldV[i].z << std::endl;
					}
					delete[] worldV;
					worldV = NULL;
					std::cout << "Save point cloud successful in PointCloud.txt" << std::endl;
					PointCloudWriter.close();
					f_bPointClound = false;
				}
				//Display the WDR Depth Image
				Opencv_Depth(wdrSlope, wdrDepthFrame.height, wdrDepthFrame.width, wdrDepthFrame.pFrameData, imageMat);
#ifdef FPS
				if (fps_tof != 0)
				{
					char fps[20];
					sprintf_s(fps, "FPS: %d", fps_tof);
					putText(imageMat, fps, Point2d(10, 20), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0, 0, 0));
				}
#endif
				cv::imshow(wdrDepthImageWindow, imageMat);
			}
			else
			{
				cout << "Ps2_GetFrame PsWDRDepthFrame status:" << status << " pFrameData is NULL " << endl;
			}
		}

		//Get RGB frame, RGB frame only output in following data mode
		if (1 == frameReady.rgb)
		{
			status = Ps2_GetFrame(deviceHandle, sessionIndex, PsRGBFrame, &rgbFrame);

			if (rgbFrame.pFrameData != NULL)
			{
#ifdef FPS
				countof_loop_rgb++;
				if (countof_loop_rgb >= FPS_LEN)
				{
					fps_rgb = 1000 * FPS_LEN / tatoldelay_rgb;
					//cout << fps_tof<<endl;
					countof_loop_rgb = 0;
					tatoldelay_rgb = 0;
				}
#endif
				//Display the RGB Image
				imageMat = cv::Mat(rgbFrame.height, rgbFrame.width, CV_8UC3, rgbFrame.pFrameData);

#ifdef FPS
				if (fps_rgb != 0)
				{
					char fps[20];
					sprintf_s(fps, "FPS: %d", fps_rgb);
					putText(imageMat, fps, Point2d(10, 20), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0, 0, 0));
				}
#endif
				cv::imshow(rgbImageWindow, imageMat);
			}
			else
			{
				cout << "Ps2_GetFrame PsRGBFrame status:" << status << " pFrameData is NULL " << endl;
			}
		}

		//Get mapped depth frame which is mapped to rgb camera space
		//Mapped depth frame only output in following data mode
		//And can only get when the feature is enabled through api Ps2_SetMapperEnabledRGBToDepth
		//When the key "L/l" pressed, this feature enable or disable
		if (1 == frameReady.mappedDepth)
		{
			status = Ps2_GetFrame(deviceHandle, sessionIndex, PsMappedDepthFrame, &mappedDepthFrame);

			if (mappedDepthFrame.pFrameData != NULL)
			{
				//Display the MappedDepth Image
				imageMat = cv::Mat(mappedDepthFrame.height, mappedDepthFrame.width, CV_16UC1, mappedDepthFrame.pFrameData);
				cv::Mat mappedDepthMat;
				imageMat.convertTo(mappedDepthMat, CV_8U, 255.0 / (f_bWDRMode ? wdrSlope : slope));
				cv::applyColorMap(mappedDepthMat, mappedDepthMat, cv::COLORMAP_RAINBOW);
				cv::imshow(mappedDepthImageWindow, mappedDepthMat);
			}
			else
			{
				cout << "Ps2_GetFrame PsMappedDepthFrame status:" << status << " pFrameData is NULL " << endl;
			}
		}

		//Get mapped rgb frame which is mapped to depth camera space
		//Mapped rgb frame only output in following data mode
		//And can only get when the feature is enabled through api Ps2_SetMapperEnabledDepthToRGB
		//When the key "Q/q" pressed, this feature enable or disable
		if (1 == frameReady.mappedRGB)
		{
			status = Ps2_GetFrame(deviceHandle, sessionIndex, PsMappedRGBFrame, &mappedRGBFrame);

			if (mappedRGBFrame.pFrameData != NULL)
			{
				//Display the MappedRGB Image
				imageMat = cv::Mat(mappedRGBFrame.height, mappedRGBFrame.width, CV_8UC3, mappedRGBFrame.pFrameData);
				cv::imshow(mappedRgbImageWindow, imageMat);
			}
			else
			{
				cout << "Ps2_GetFrame PsMappedRGBFrame status:" << status << " pFrameData is NULL " << endl;
			}
		}
	KEY:
		unsigned char key = waitKey(1);
		imageMat.release();

		if (key == 'M' || key == 'm')
		{
			cout << "Selection: 0:DepthAndRgb_30; 1:IrAndRgb_30; 2:PsDepthAndIRAndRGB_30; 3:WDR_Depth" << endl;

			int index = -1;
			cin >> index;
			//clear buffer and cin flag. if not, cin will not get input anymore;
			if (cin.fail())
			{
				std::cout << "Unexpected input\n";
				cin.clear();
				cin.ignore(1024, '\n');
				continue;
			}
			else
			{
				cin.clear();
				cin.ignore(1024, '\n');
			}
			PsDataMode t_datamode = PsDepthAndIRAndRGB_30;

			switch (index)
			{
			case 0:
				t_datamode = PsDepthAndRGB_30;
				break;
			case 1:
				t_datamode = PsIRAndRGB_30;
				break;
			case 2:
				t_datamode = PsDepthAndIRAndRGB_30;
				break;
			case 3:
				t_datamode = PsWDR_Depth;
				break;
			default:
				cout << "Unsupported data mode!" << endl;
				continue;
			}

			if (t_datamode == PsWDR_Depth)
			{
				Ps2_SetWDROutputMode(deviceHandle, sessionIndex, &wdrMode);
				f_bWDRMode = true;

			}
			else
			{
				f_bWDRMode = false;
			}
			if (dataMode == PsWDR_Depth&&t_datamode != PsWDR_Depth)
			{
				status = Ps2_GetDepthRange(deviceHandle, sessionIndex, &depthRange);
				cout << "Get depth range," << " depthRange: " << depthRange << endl;
				if (status != PsRetOK)
				{
					cout << "Get depth range failed! " << endl;
				}
				else
				{
					status = Ps2_GetMeasuringRange(deviceHandle, sessionIndex, depthRange, &measuringrange);
					if (status != PsReturnStatus::PsRetOK)
						cout << "Ps2_GetMeasuringRange failed!" << endl;
					else
					{
						switch (depthRange)
						{
						case PsNearRange:
						case PsXNearRange:
						case PsXXNearRange:
							slope = measuringrange.effectDepthMaxNear;
							break;

						case PsMidRange:
						case PsXMidRange:
						case PsXXMidRange:
							slope = measuringrange.effectDepthMaxMid;
							break;

						case PsFarRange:
						case PsXFarRange:
						case PsXXFarRange:

							slope = measuringrange.effectDepthMaxFar;
							break;
						default:
							break;
						}
						cout << "slope  ==  " << slope << endl;
					}
				}
			}

			status = Ps2_SetDataMode(deviceHandle, sessionIndex, (PsDataMode)t_datamode);
			if (status != PsRetOK)
			{
				cout << "Ps2_SetDataMode  status" << status << endl;
			}
			else
			{
				dataMode = t_datamode;
			}
			destroycount = 3;
#ifdef FPS
			delayT = 0;

			countof_loop_tof = 0;
			tatoldelay_tof = 0;
			fps_tof = 0;

			countof_loop_rgb = 0;
			tatoldelay_rgb = 0;
			fps_rgb = 0;

			countof_loop_ir = 0;
			tatoldelay_ir = 0;
			fps_ir = 0;

			countof_loop_wdr1 = 0;
			tatoldelay_wdr1 = 0;
			fps_wdr1 = 0;

			countof_loop_wdr2 = 0;
			tatoldelay_wdr2 = 0;
			fps_wdr2 = 0;

			countof_loop_wdr3 = 0;
			tatoldelay_wdr3 = 0;
			fps_wdr3 = 0;
#endif
		}
		else if ((key == '0') || (key == '1') || (key == '2') || (key == '3') || (key == '4') || (key == '5') || (key == '6') || (key == '7') || (key == '8'))
		{
			switch (key)
			{
			case '0':
				depthRange = PsNearRange;
				slope = 1450;
				break;
			case '1':
				depthRange = PsMidRange;
				slope = 3000;
				break;
			case '2':
				depthRange = PsFarRange;
				slope = 4400;
				break;
			case '3':
				depthRange = PsXNearRange;
				slope = 4800;
				break;
			case '4':
				depthRange = PsXMidRange;
				slope = 5600;
				break;
			case '5':
				depthRange = PsXFarRange;
				slope = 7500;
				break;
			case '6':
				depthRange = PsXXNearRange;
				slope = 9600;
				break;
			case '7':
				depthRange = PsXXMidRange;
				slope = 11200;
				break;
			case '8':
				depthRange = PsXXFarRange;
				slope = 15000;
				break;
			default:
				cout << "Unsupported Range!" << endl;
				continue;
			}
			status = Ps2_SetDepthRange(deviceHandle, sessionIndex, depthRange);
			if (depthRange == PsNearRange)
				cout << "Set depth range to Near," << " status: " << status << endl;
			else if (depthRange == PsMidRange)
				cout << "Set depth range to Mid," << " status: " << status << endl;
			else if (depthRange == PsFarRange)
				cout << "Set depth range to Far," << " status: " << status << endl;
			else if (depthRange == PsXNearRange)
				cout << "Set depth range to XNearRange," << " status: " << status << endl;
			else if (depthRange == PsXMidRange)
				cout << "Set depth range to XMidRange," << " status: " << status << endl;
			else if (depthRange == PsXFarRange)
				cout << "Set depth range to XFarRange," << " status: " << status << endl;
			else if (depthRange == PsXXNearRange)
				cout << "Set depth range to XXNearRange," << " status: " << status << endl;
			else if (depthRange == PsXXMidRange)
				cout << "Set depth range to XXMidRange," << " status: " << status << endl;
			else if (depthRange == PsXXFarRange)
				cout << "Set depth range to XXFarRange," << " status: " << status << endl;

			if (status != PsRetOK)
			{
				cout << "Set depth range failed! " << endl;
			}

			status = Ps2_GetDepthRange(deviceHandle, sessionIndex, &depthRange);
			cout << "Get depth range," << " depthRange: " << depthRange << endl;
			if (status != PsRetOK)
			{
				cout << "Get depth range failed! " << endl;
			}
			else
			{
				status = Ps2_GetMeasuringRange(deviceHandle, sessionIndex, depthRange, &measuringrange);
				if (status != PsReturnStatus::PsRetOK)
					cout << "Ps2_GetMeasuringRange failed!" << endl;
				else
				{
					switch (depthRange)
					{
					case PsNearRange:
					case PsXNearRange:
					case PsXXNearRange:
						slope = measuringrange.effectDepthMaxNear;
						break;

					case PsMidRange:
					case PsXMidRange:
					case PsXXMidRange:
						slope = measuringrange.effectDepthMaxMid;
						break;

					case PsFarRange:
					case PsXFarRange:
					case PsXXFarRange:

						slope = measuringrange.effectDepthMaxFar;
						break;
					default:
						break;
					}
					cout << "slope  ==  " << slope << endl;
				}
			}
		}
		else if (key == 'P' || key == 'p')
		{
			if (dataMode != PsIRAndRGB_30)
			{
				f_bPointClound = true;
			}
			else
			{
				cout << "has no depth" << endl;
			}
		}
		else if (key == 'T' || key == 't')
		{
			//Set background filter threshold
			static uint16_t threshold = 0;
			threshold += 10;
			if (threshold > 100)
				threshold = 0;
			status = Ps2_SetThreshold(deviceHandle, sessionIndex, threshold);

			if (PsRetOK == status)
			{
				cout << "Set background threshold value: " << threshold << endl;
			}
			else
			{
				cout << "Set background threshold error,check if the datamode is WDR mode or the confidence filter is opened" << endl;
			}
		}

		else if (key == 'V' || key == 'v')
		{
			static bool bWDRStyle = true;
			status = Ps2_SetWDRStyle(deviceHandle, sessionIndex, bWDRStyle ? PsWDR_ALTERNATION : PsWDR_FUSION);
			if (PsRetOK == status)
			{
				cout << "WDR image output " << (bWDRStyle ? "alternatively in multi range." : "Fusion.") << endl;
				bWDRStyle = !bWDRStyle;
			}
			else
			{
				cout << "Set WDR Style " << (bWDRStyle ? PsWDR_ALTERNATION : PsWDR_FUSION) << "  satus : " << status << endl;
			}
#ifdef FPS
			delayT = 0;

			countof_loop_tof = 0;
			tatoldelay_tof = 0;
			fps_tof = 0;

			countof_loop_rgb = 0;
			tatoldelay_rgb = 0;
			fps_rgb = 0;

			countof_loop_ir = 0;
			tatoldelay_ir = 0;
			fps_ir = 0;

			countof_loop_wdr1 = 0;
			tatoldelay_wdr1 = 0;
			fps_wdr1 = 0;

			countof_loop_wdr2 = 0;
			tatoldelay_wdr2 = 0;
			fps_wdr2 = 0;

			countof_loop_wdr3 = 0;
			tatoldelay_wdr3 = 0;
			fps_wdr3 = 0;
#endif
		}
		else if (key == 27)	//ESC Pressed
		{
			break;
		}
		else if (key == 'S' || key == 's')
		{
			status = Ps2_SetSynchronizeEnabled(deviceHandle, sessionIndex, f_bSync);
			if (status == PsRetOK)
			{
				cout << "Set Synchronize " << (f_bSync ? "Enabled." : "Disabled.") << endl;
				f_bSync = !f_bSync;
			}
		}
		else if (key == 'R' || key == 'r')
		{
			cout << "please select RGB resolution to set: 0:1600x1200; 1:800x600; 2:640x480" << endl;
			int index = 0;
			cin >> index;
			if (cin.fail())
			{
				std::cout << "Unexpected input\n";
				cin.clear();
				cin.ignore(1024, '\n');
				continue;
			}
			else
			{
				cin.clear();
				cin.ignore(1024, '\n');
			}
			PsResolution resolution = PsRGB_Resolution_1600_1200;
			switch (index)
			{
			case 0:
				resolution = PsRGB_Resolution_1600_1200;
				break;
			case 1:
				resolution = PsRGB_Resolution_800_600;
				break;
			case 2:
				resolution = PsRGB_Resolution_640_480;
				break;
			default:
				break;
			}
			Ps2_SetRGBResolution(deviceHandle, sessionIndex, resolution);
		}
		else if (key == 'L' || key == 'l')
		{
			status = Ps2_SetMapperEnabledRGBToDepth(deviceHandle, sessionIndex, f_bMappedDepth);
			if (status == PsRetOK)
			{
				cout << "Set Mapper RGBToDepth " << (f_bMappedDepth ? "Enabled." : "Disabled.") << endl;
				f_bMappedDepth = !f_bMappedDepth;
			}
		}
		else if (key == 'Q' || key == 'q')
		{
			status = Ps2_SetMapperEnabledDepthToRGB(deviceHandle, sessionIndex, f_bMappedRGB);
			if (status == PsRetOK)
			{
				cout << "Set Mapper DepthToRGB " << (f_bMappedRGB ? "Enabled." : "Disabled.") << endl;
				f_bMappedRGB = !f_bMappedRGB;
			}
		}
		else if (key == 'C' || key == 'c')
		{
			status = Ps2_SetConfidenceFilterEnabled(deviceHandle, sessionIndex, f_bConfidence);
			if (PsRetOK == status)
			{
				cout << "Set Confidence Filter " << (f_bConfidence ? "true" : "false") << endl;
			}
			else
			{
				cout << "Set Confidence Filter failed, status:" << status << endl;
			}
			f_bConfidence = !f_bConfidence;
		}
		else if (key == 'F' || key == 'f')
		{
			static uint16_t conthreshold = 0;
			conthreshold += 10;
			if (conthreshold > 300)
				conthreshold = 0;

			status = Ps2_SetConfidenceFilterThreshold(deviceHandle, sessionIndex, conthreshold);

			if (PsRetOK == status)
			{
				cout << "Set Confidence Filter Threshold  value: " << conthreshold << endl;
			}
			else
			{
				cout << "Set Confidence Filter Threshold failed, status:" << status << endl;
			}
		}
		else if (key == 'H' || key == 'h')
		{
			static PsWDRConfidenceThreshold  conthreshold = { 0 };
			conthreshold.threshold1 += 10;
			if (conthreshold.threshold1 > 100)
				conthreshold.threshold1 = 0;
			conthreshold.threshold2 += 20;
			if (conthreshold.threshold2 > 300)
				conthreshold.threshold2 = 0;
			conthreshold.threshold3 += 30;
			if (conthreshold.threshold3 > 600)
				conthreshold.threshold3 = 0;

			status = Ps2_SetWDRConfidenceFilterThreshold(deviceHandle, sessionIndex, conthreshold);

			if (PsRetOK == status)
			{
				cout << "Set WDR Confidence Filter Threshold value: " << status
					<< "  ,  " << conthreshold.threshold1
					<< "  ,  " << conthreshold.threshold2
					<< "  ,  " << conthreshold.threshold3
					<< endl;
			}
			else
			{
				cout << "Set WDR Confidence Filter Threshold failed, status:" << status << endl;
			}
		}
		else if (key == 'B' || key == 'b')
		{
			static bool bmanualEnabled = true;

			status = Ps2_SetRGBManualExposureEnabled(deviceHandle, sessionIndex, bmanualEnabled);
			cout << "SetRGBExposureType: " << status << "  ,   " << bmanualEnabled << endl;
			bmanualEnabled = !bmanualEnabled;
		}
		else if (key == 'N' || key == 'n')
		{
			static uint16_t expore = 1;
			expore += 100;
			if (expore > 4000)
				expore = 1;
			status = Ps2_SetRGBAbsoluteExposure(deviceHandle, sessionIndex, expore);

			if (PsRetOK == status)
			{
				cout << "Ps2_SetRGBAbsoluteExposure success , value: " << expore << endl;
			}
			else
			{
				cout << "Ps2_SetRGBAbsoluteExposure failed, status: " << status << endl;
			}
		}
	}

	status = Ps2_CloseDevice(&deviceHandle);
	cout << "CloseDevice status: " << status << endl;

	status = Ps2_Shutdown();
	cout << "Shutdown status: " << status << endl;
	cv::destroyAllWindows();

	return 0;
}

void HotPlugStateCallback(const PsDeviceInfo* pInfo, int status)
{
	cout << "uri " << status << "  " << pInfo->uri << "    " << (status == 0 ? "add" : "remove") << endl;
	cout << "alia " << status << "  " << pInfo->alias << "    " << (status == 0 ? "add" : "remove") << endl;
}
