#include "rmFourier.h"

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
	PF_ADD_LAYER("Select layer", PF_LayerDefault_NONE, CHECK_LAYER_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOX("Inverse", "Calculate inverse Fourier transform", FALSE, 0, INVERSE_FFT_DISK_ID);
	
	out_data->num_params = RMFOURIER_NUM_PARAMS;

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
	PF_ParamDef inverseFftParam, checkoutLayerParam;
	PF_RenderRequest req = extra->input->output_request;
	PF_CheckoutResult in_result;

	AEGP_SuiteHandler suites(in_data->pica_basicP);

	PF_Handle	infoH = suites.HandleSuite1()->host_new_handle(sizeof(rmFourierInfo));

	if (infoH) {

		rmFourierInfo	*infoP = reinterpret_cast<rmFourierInfo*>(suites.HandleSuite1()->host_lock_handle(infoH));

		if (infoP) {
			extra->output->pre_render_data = infoH;

			// Params here
			AEFX_CLR_STRUCT(inverseFftParam);
			ERR(PF_CHECKOUT_PARAM(
				in_data,
				RMFOURIER_CHECK_LAYER,
				in_data->current_time,//(in_data->current_time + params[CHECK_FRAME]->u.sd.value * in_data->time_step),
				in_data->time_step,
				in_data->time_scale,
				&checkoutLayerParam));

			AEFX_CLR_STRUCT(inverseFftParam);
			ERR(PF_CHECKOUT_PARAM(
				in_data,
				RMFOURIER_INVERSE_FFT,
				in_data->current_time,
				in_data->time_step,
				in_data->time_scale,
				&inverseFftParam
			));

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
					infoP->inverseCB 	= inverseFftParam.u.bd.value;
					infoP->imgWidth		= in_data->width;
					infoP->imgHeight	= in_data->height;
					infoP->phaseLayer	= &checkoutLayerParam.u.ld;

					UnionLRect(&in_result.result_rect, &extra->output->result_rect);
					UnionLRect(&in_result.max_result_rect, &extra->output->max_result_rect);
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

					case PF_PixelFormat_ARGB128: {
						A_long imgSize = input_worldP->width * input_worldP->height;

						if (!infoP->inverseCB) {
							//AEGP_IterateGeneric
							infoP->imgVectorR.resize(imgSize);
							infoP->imgVectorG.resize(imgSize);
							infoP->imgVectorB.resize(imgSize);

							// Make a vector from the image pixels.
							ERR(suites.IterateFloatSuite1()->iterate(
								in_data,
								0,							// progress base
								output_worldP->height,		// progress final
								infoP->phaseLayer,				// src
								NULL,						// area - null for all pixels
								(void*)infoP,				// custom data pointer
								pixelToVector,				// pixel function pointer
								output_worldP
							));

							
						}
						
						// -------------- Let's transform every row using thread --------------
						const unsigned int numOfThreads = 4;// 16;
						std::thread threads[numOfThreads];
						int thPernTh = infoP->imgHeight / numOfThreads;
						int remainderTh = infoP->imgHeight % numOfThreads;

						for (int count = 0; count < thPernTh; count++) {
							for (int nTh = 0; nTh < numOfThreads; nTh++) {
								A_long currentRow = (count*numOfThreads) + nTh;

								threads[nTh] = std::thread(transformRow, &infoP->imgVectorR, &infoP->imgVectorG, &infoP->imgVectorB, currentRow, infoP->imgWidth, infoP->inverseCB);
							}

							for (unsigned int nTh = 0; nTh < numOfThreads; nTh++) {
								threads[nTh].join();
							}
						}

						// Let's compute the remainder
						for (int count = 0; count < remainderTh; count++) {
							A_long currentRow = (thPernTh*numOfThreads) + count;
							transformRow(&infoP->imgVectorR, &infoP->imgVectorG, &infoP->imgVectorB, currentRow, infoP->imgWidth, infoP->inverseCB);
						}

						// -------------- Let's trasform every column with thread --------------
						thPernTh = infoP->imgWidth / numOfThreads;
						remainderTh = infoP->imgWidth % numOfThreads;

						for (int count = 0; count < thPernTh; count++) {
							for (int nTh = 0; nTh < numOfThreads; nTh++) {
								A_long currentColumn = (count*numOfThreads) + nTh;

								threads[nTh] = std::thread(transformColumn, &infoP->imgVectorR, &infoP->imgVectorG, &infoP->imgVectorB, currentColumn, infoP->imgWidth, infoP->imgHeight, infoP->inverseCB);
							}

							for (unsigned int nTh = 0; nTh < numOfThreads; nTh++) {
								threads[nTh].join();
							}
						}

						// Let's compute the remainder
						for (int count = 0; count < remainderTh; count++) {
							A_long currentColumn = (thPernTh*numOfThreads) + count;
							transformColumn(&infoP->imgVectorR, &infoP->imgVectorG, &infoP->imgVectorB, currentColumn, infoP->imgWidth, infoP->imgHeight, infoP->inverseCB);
						}
						
						if (!infoP->inverseCB) {
							infoP->fftComputed = true;
						}

						if (infoP->fftComputed) {
							// Put the values in the vector back to the image, also get the max in order to normalize later
							ERR(suites.IterateFloatSuite1()->iterate(
								in_data,
								0,							// progress base
								output_worldP->height,		// progress final
								input_worldP,				// src
								NULL,						// area - null for all pixels
								(void*)infoP,				// custom data pointer
								vectorToPixel,				// pixel function pointer
								output_worldP
							));
						}

						if (!infoP->inverseCB) {
							// Normalize the image
							ERR(suites.IterateFloatSuite1()->iterate(
								in_data,
								0,							// progress base
								output_worldP->height,		// progress final
								input_worldP,				// src
								NULL,						// area - null for all pixels
								(void*)infoP,				// custom data pointer
								normalizeImg,				// pixel function pointer
								output_worldP
							));

							// Circular shift
							infoP->tmpOutput = output_worldP;
							ERR(suites.IterateFloatSuite1()->iterate(
								in_data,
								0,							// progress base
								output_worldP->height,		// progress final
								input_worldP,				// src
								NULL,						// area - null for all pixels
								(void*)infoP,				// custom data pointer
								circularShift,				// pixel function pointer
								output_worldP
							));
						}
						

						break;
					}
						

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

