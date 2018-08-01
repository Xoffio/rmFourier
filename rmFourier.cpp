#include "rmFourier.h"
#include "FftComplex.hpp"
#include "math.h"
#include <vector>
#include <cmath>

static PF_Err 
About (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	suites.ANSICallbacksSuite1()->sprintf(	out_data->return_msg,
											"%s v%d.%d\r%s",
											strName.c_str(),
											MAJOR_VERSION, 
											MINOR_VERSION, 
											strDescription.c_str());
	return PF_Err_NONE;
}

static PF_Err 
GlobalSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	out_data->my_version = PF_VERSION(	MAJOR_VERSION, 
										MINOR_VERSION,
										BUG_VERSION, 
										STAGE_VERSION, 
										BUILD_VERSION);

	out_data->out_flags |=	PF_OutFlag_PIX_INDEPENDENT |
							PF_OutFlag_USE_OUTPUT_EXTENT;

	out_data->out_flags2 =	PF_OutFlag2_SUPPORTS_SMART_RENDER |
							PF_OutFlag2_FLOAT_COLOR_AWARE;
	
	return PF_Err_NONE;
}

static PF_Err 
ParamsSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err		err		= PF_Err_NONE;
	PF_ParamDef	def;	

	AEFX_CLR_STRUCT(def);

	// RM-NOTE Here you make the parameters setup
	
	out_data->num_params = RMFOURIER_NUM_PARAMS;

	return err;
}

static PF_PixelFloat
*getXY(PF_EffectWorld &def, int x, int y) {
	return (PF_PixelFloat*)((char*)def.data +
		(y * def.rowbytes) +
		(x * sizeof(PF_PixelFloat)));

}

PF_EffectWorld tmpFourier(PF_EffectWorld inWorld) {
	std::vector<std::complex<double>>	imgRedDataVector, imgGreenDataVector, imgBlueDataVector, finalImgGreenDataVector;
	std::vector<double>					copyImgRedDataVector, copyImgGreenDataVector, copyImgBlueDataVector;
	int imgArraySize = inWorld.height * inWorld.width;

	// Convert the raw images to vectors
	PF_PixelFloat* pixelPointerAtIndex;
	for (unsigned long index= 0; index < imgArraySize; index++){
		pixelPointerAtIndex = (PF_PixelFloat*)((char*)inWorld.data + (index * sizeof(PF_PixelFloat)));
		imgRedDataVector.push_back( (std::complex<double>) pixelPointerAtIndex->red);
		//imgGreenDataVector.push_back((std::complex<double>) pixelPointerAtIndex->green);
		imgBlueDataVector.push_back((std::complex<double>) pixelPointerAtIndex->blue);
	}

	// Transform rows
	for (int row = 0; row < inWorld.height; row++) {
		std::vector<std::complex<double>> tmpRedVector, tmpGreenVector, tmpBlueVector;
		tmpRedVector.clear();
		tmpGreenVector.clear();
		tmpBlueVector.clear();
		for (int col = 0; col < inWorld.width; col++) {
			unsigned long pointAt = (row * inWorld.width) + col;
			pixelPointerAtIndex = (PF_PixelFloat*)((char*)inWorld.data + ( pointAt * sizeof(PF_PixelFloat)));
			//tmpRedVector.push_back((std::complex<double>) pixelPointerAtIndex->red);
			tmpGreenVector.push_back((std::complex<double>) pixelPointerAtIndex->green);
			//tmpBlueVector.push_back((std::complex<double>) pixelPointerAtIndex->blue);
			//finalImgRedDataVector.push_back(0);
			finalImgGreenDataVector.push_back(0);
			//finalImgBlueDataVector.push_back(0);
		}

		// Fourier Transform Row
		//fft::transform(tmpRedVector);
		fft::transform(tmpGreenVector);
		//fft::transform(tmpBlueVector);

		//imgRedDataVector.insert(std::end(imgRedDataVector), std::begin(tmpRedVector), std::end(tmpRedVector));
		imgGreenDataVector.insert(std::end(imgGreenDataVector), std::begin(tmpGreenVector), std::end(tmpGreenVector));
		//imgBlueDataVector.insert(std::end(imgBlueDataVector), std::begin(tmpBlueVector), std::end(tmpBlueVector));
	}

	// Transform Columns
	for (int col = 0; col < inWorld.width; col++) {
		std::vector<std::complex<double>> tmpRedVector, tmpGreenVector, tmpBlueVector;
		tmpRedVector.clear();
		tmpGreenVector.clear();
		tmpBlueVector.clear();
		for (int row = 0; row < inWorld.height; row++) {
			unsigned long pointAt = (row * inWorld.width) + col;
			//tmpRedVector.push_back(imgRedDataVector[pointAt]);
			tmpGreenVector.push_back(imgGreenDataVector[pointAt]);
			//tmpBlueVector.push_back(imgBlueDataVector[pointAt]);
		}
		// Fourier Transform Row
		//fft::transform(tmpRedVector);
		fft::transform(tmpGreenVector);
		//fft::transform(tmpBlueVector);
		for (int row = 0; row < inWorld.height; row++) {
			unsigned long pointAt = (row * inWorld.width) + col;
			//finalImgRedDataVector[pointAt] = tmpRedVector[row];
			finalImgGreenDataVector[pointAt] = tmpGreenVector[row];
			//finalImgBlueDataVector[pointAt] = tmpBlueVector[row];
		}
	}

	// Fourier Transform vectors
	fft::transform(imgRedDataVector);
	fft::transform(imgGreenDataVector);
	fft::transform(imgBlueDataVector);

	// Copy the Fourier data from the vectors back to the image
	double rMaxVal = 0;
	double gMaxVal = 0;
	double bMaxVal = 0;
	for (unsigned long index = 0; index < imgArraySize; index++) {
		pixelPointerAtIndex = (PF_PixelFloat*)((char*)inWorld.data + (index * sizeof(PF_PixelFloat)));
		pixelPointerAtIndex->red	= log(1.0 + abs(imgRedDataVector[index]) );
		pixelPointerAtIndex->green	= log(1.0 + abs(finalImgGreenDataVector[index]) );
		pixelPointerAtIndex->blue	= log(1.0 + abs(imgBlueDataVector[index]) );

		// Get max value
		if (pixelPointerAtIndex->red > rMaxVal) rMaxVal = pixelPointerAtIndex->red;
		if (pixelPointerAtIndex->green > gMaxVal) gMaxVal = pixelPointerAtIndex->green;
		if (pixelPointerAtIndex->blue > bMaxVal) bMaxVal = pixelPointerAtIndex->blue;
	}

	// Normalize values
	for (unsigned long index = 0; index < imgArraySize; index++) {
		pixelPointerAtIndex = (PF_PixelFloat*)((char*)inWorld.data + (index * sizeof(PF_PixelFloat)));
		pixelPointerAtIndex->red = pixelPointerAtIndex->red / rMaxVal;
		pixelPointerAtIndex->green = pixelPointerAtIndex->green / gMaxVal;
		pixelPointerAtIndex->blue = pixelPointerAtIndex->blue / bMaxVal;

		// Make a copy
		copyImgRedDataVector.push_back(pixelPointerAtIndex->red);
		copyImgGreenDataVector.push_back(pixelPointerAtIndex->green);
		copyImgBlueDataVector.push_back(pixelPointerAtIndex->blue);
	}

	// Circular shift
	for (int row = 0; row < inWorld.height; row++) {
		for (int col = 0; col < inWorld.width; col++) {
			PF_PixelFloat currentP;
			PF_Point srcP = {0, (inWorld.height - 1 - row) };

			if (col < (inWorld.width/2)) {
				srcP.h = col + (inWorld.width / 2);
			}
			else {
				srcP.h = col - (inWorld.width / 2);
			}

			if (row < (inWorld.height / 2)) {
				srcP.v = row + (inWorld.height / 2);
			}
			else {
				srcP.v = row - (inWorld.width / 2);
			}

			unsigned long dstPointAt = (row * inWorld.width) + col;
			unsigned long srcPointAt = (srcP.v * inWorld.width) + srcP.h;

			pixelPointerAtIndex = (PF_PixelFloat*)((char*)inWorld.data + (dstPointAt * sizeof(PF_PixelFloat)));

			pixelPointerAtIndex->red = copyImgRedDataVector[srcPointAt];
			pixelPointerAtIndex->green = copyImgGreenDataVector[srcPointAt];
			//pixelPointerAtIndex->blue = copyImgBlueDataVector[srcPointAt];
		}
	}

	return(inWorld);
}

static PF_Err
fourier32(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP)
{
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	PF_Err				err = PF_Err_NONE;
	PF_Fixed			new_xFi = 0,
						new_yFi = 0;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);
	

	// Math.sqrt( Math.pow( re.data[i], 2 ) + Math.pow( im.data[i], 2 ) );
	outP->alpha = inP->alpha;
	outP->red = sqrt( pow(inP->red, 2) + pow(inP->red, 2) );
	outP->green = sqrt(pow(inP->green, 2) + pow(inP->green, 2));
	outP->blue = sqrt(pow(inP->blue, 2) + pow(inP->blue, 2));

	return err;
}

static PF_Err 
Render (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err				err		= PF_Err_NONE;
	AEGP_SuiteHandler	suites(in_data->pica_basicP);

	return err;
}

static PF_Err
PreRender(
	PF_InData			*in_data,
	PF_OutData			*out_data,
	PF_PreRenderExtra	*extra)
{
	PF_Err err = PF_Err_NONE;
	PF_ParamDef params;
	PF_RenderRequest req = extra->input->output_request;
	PF_CheckoutResult in_result;

	AEGP_SuiteHandler suites(in_data->pica_basicP);

	PF_Handle	infoH = suites.HandleSuite1()->host_new_handle(sizeof(rmFourierInfo));

	if (infoH) {

		rmFourierInfo	*infoP = reinterpret_cast<rmFourierInfo*>(suites.HandleSuite1()->host_lock_handle(infoH));

		if (infoP) {
			extra->output->pre_render_data = infoH;

			// Params here
			AEFX_CLR_STRUCT(params);

			if (!err) {
				req.preserve_rgb_of_zero_alpha = FALSE;	
				req.field = PF_Field_FRAME;				

				ERR(extra->cb->checkout_layer(
					in_data->effect_ref,
					RMFOURIER_INPUT,
					RMFOURIER_INPUT,
					&req,
					in_data->current_time,
					in_data->time_step,
					in_data->time_scale,
					&in_result
				));

				if (!err) {
					AEFX_CLR_STRUCT(*infoP);
					// Here you get the input values
					//infoP->blend_valFi 	= blend_param.u.fd.value;


					UnionLRect(&in_result.result_rect, &extra->output->result_rect);
					UnionLRect(&in_result.max_result_rect, &extra->output->max_result_rect);

					//	Notice something missing, namely the PF_CHECKIN_PARAM to balance
					//	the old-fashioned PF_CHECKOUT_PARAM, above? 

					//	For SmartFX, AE automagically checks in any params checked out 
					//	during PF_Cmd_SMART_PRE_RENDER, new or old-fashioned.
				}
			}

			suites.HandleSuite1()->host_unlock_handle(infoH);
		}
		else {
			err = PF_Err_OUT_OF_MEMORY;
		}
	}
	else {
		err = PF_Err_OUT_OF_MEMORY;
	}
	return err;
}


static PF_Err
SmartRender(
	PF_InData				*in_data,
	PF_OutData				*out_data,
	PF_SmartRenderExtra		*extra)
{

	PF_Err				err = PF_Err_NONE,
						err2 = PF_Err_NONE;

	AEGP_SuiteHandler 	suites(in_data->pica_basicP);
	PF_EffectWorld		*input_worldP = NULL,
						*output_worldP = NULL;
	PF_WorldSuite2		*wsP = NULL;
	PF_PixelFormat		format = PF_PixelFormat_INVALID;

	PF_Point			origin;

	rmFourierInfo	*infoP = reinterpret_cast<rmFourierInfo*>(suites.HandleSuite1()->host_lock_handle(reinterpret_cast<PF_Handle>(extra->input->pre_render_data)));

	if (infoP) {
		if (!infoP->no_opB) {
			// checkout input & output buffers.
			ERR((extra->cb->checkout_layer_pixels(in_data->effect_ref, RMFOURIER_INPUT, &input_worldP)));

			ERR(extra->cb->checkout_output(in_data->effect_ref, &output_worldP));

			if (!err && output_worldP) {
				ERR(AEFX_AcquireSuite(in_data,
					out_data,
					kPFWorldSuite,
					kPFWorldSuiteVersion2,
					"Couldn't load suite.",
					(void**)&wsP));

				infoP->ref = in_data->effect_ref;
				infoP->samp_pb.src = input_worldP;
				infoP->in_data = *in_data;

				ERR(wsP->PF_GetPixelFormat(input_worldP, &format));

				origin.h = (A_short)(in_data->output_origin_x);
				origin.v = (A_short)(in_data->output_origin_y);

				if (!err) {
					switch (format) {

					case PF_PixelFormat_ARGB128:
						*output_worldP = tmpFourier(*input_worldP);
						/*ERR(suites.IterateFloatSuite1()->iterate(in_data,
							0,                        // progress base
							output_worldP->height,    // progress final
							input_worldP,             // src
							NULL,                     // area - null for all pixels
							(void*)infoP,              // custom data pointer
							fourier32,        // pixel function pointer
							output_worldP));*/
						break;

					case PF_PixelFormat_ARGB64:
						break;

					case PF_PixelFormat_ARGB32:
						break;

					default:
						err = PF_Err_BAD_CALLBACK_PARAM;
						break;
					}
				}
			}
		}
		else {
			// copy input buffer;
			ERR(PF_COPY(input_worldP, output_worldP, NULL, NULL));
		}

		suites.HandleSuite1()->host_unlock_handle(reinterpret_cast<PF_Handle>(extra->input->pre_render_data));

	}
	else {
		err = PF_Err_BAD_CALLBACK_PARAM;
	}

	ERR2(AEFX_ReleaseSuite(
		in_data,
		out_data,
		kPFWorldSuite,
		kPFWorldSuiteVersion2,
		"Couldn't release suite."
	));

	return err;
}

DllExport	
PF_Err 
EntryPointFunc (
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err		err = PF_Err_NONE;
	
	try {
		switch (cmd) {
			case PF_Cmd_ABOUT:
				err = About(in_data, out_data, params, output);
				break;
				
			case PF_Cmd_GLOBAL_SETUP:
				err = GlobalSetup(in_data, out_data, params, output);
				break;
				
			case PF_Cmd_PARAMS_SETUP:
				err = ParamsSetup(in_data, out_data, params, output);
				break;
				
			case PF_Cmd_SMART_PRE_RENDER:
				err = PreRender(in_data, out_data, (PF_PreRenderExtra*)extra);
				break;

			case PF_Cmd_SMART_RENDER:
				err = SmartRender(in_data, out_data, (PF_SmartRenderExtra*)extra);
				break;
		}
	}
	catch(PF_Err &thrown_err){
		err = thrown_err;
	}
	return err;
}

