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
							PF_OutFlag_USE_OUTPUT_EXTENT |
							PF_OutFlag_SEND_UPDATE_PARAMS_UI;

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
	def.flags = PF_ParamFlag_SUPERVISE;
	PF_ADD_LAYER("Select layer", PF_LayerDefault_NONE, PHASE_LAYER_DISK_ID);

	AEFX_CLR_STRUCT(def);
	def.flags = PF_ParamFlag_SUPERVISE | PF_ParamFlag_CANNOT_TIME_VARY;
	PF_ADD_CHECKBOX("Inverse", "Calculate inverse Fourier transform", FALSE, 0, INVERSE_FFT_DISK_ID);

	AEFX_CLR_STRUCT(def);
	def.flags = PF_ParamFlag_SUPERVISE | PF_ParamFlag_CANNOT_TIME_VARY;
	PF_ADD_CHECKBOX("Phase", "Get the phase from the Fourier transform", FALSE, 0, FFT_PHASE_DISK_ID);

	AEFX_CLR_STRUCT(def);
	def.flags = PF_ParamFlag_CANNOT_TIME_VARY;
	PF_ADD_POPUP("Mode to compute",
		5,
		1,
		"RGB | Red | Green | Blue | Grayscale",
		COLOR_MODE_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("Normalize value", 0, FLT_MAX, 0, 10000, 0, 0, 2, 0, 0, NORMALIZE_DISK_ID);
	
	out_data->num_params = RMFOURIER_NUM_PARAMS;

	return err;
}

static PF_Err
UserChangedParam(
	PF_InData						*in_data,
	PF_OutData						*out_data,
	PF_ParamDef						*params[],
	PF_LayerDef						*outputP,
	const PF_UserChangedParamExtra	*which_hitP)
{
	PF_Err				err = PF_Err_NONE;
	AEGP_SuiteHandler	suites(in_data->pica_basicP);

	// If I have the inverseCB activated. Then, desactivate the pahseCB
	if (which_hitP->param_index == RMFOURIER_INVERSE_FFT || which_hitP->param_index == RMFOURIER_PHASE_LAYER){
		if (params[RMFOURIER_INVERSE_FFT]->u.bd.value == TRUE){

			if (params[RMFOURIER_FFT_PHASE]->u.bd.value == TRUE) {
				params[RMFOURIER_FFT_PHASE]->u.bd.value = FALSE;
				params[RMFOURIER_FFT_PHASE]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
				params[RMFOURIER_FFT_PHASE]->ui_flags &= ~PF_PUI_DISABLED;
			}

			PF_ParamDef checkout;
			AEFX_CLR_STRUCT(checkout);
			ERR(PF_CHECKOUT_PARAM(
				in_data,
				RMFOURIER_PHASE_LAYER,
				in_data->current_time,
				in_data->time_step,
				in_data->time_scale,
				&checkout
			));

			if (!checkout.u.ld.data) {
				params[RMFOURIER_INVERSE_FFT]->u.bd.value = FALSE;
				params[RMFOURIER_INVERSE_FFT]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;

				auto ansiSuite = AEFX_SuiteScoper<PF_ANSICallbacksSuite1>(in_data, kPFANSISuite, kPFANSISuiteVersion1);
				ansiSuite->sprintf(out_data->return_msg, "Please select a 'Phase layer' to get the inverse fft.");
			}

			err = PF_CHECKIN_PARAM(in_data, &checkout);
		}
	}

	// If I have the pahseCB activated. Then, desactivate the inverseCB
	if (which_hitP->param_index == RMFOURIER_FFT_PHASE) {
		if (params[RMFOURIER_FFT_PHASE]->u.bd.value == TRUE) {
			params[RMFOURIER_INVERSE_FFT]->u.bd.value = FALSE;
			params[RMFOURIER_INVERSE_FFT]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
		}
	}

	return err;
}

/*static PF_Err
UpdateParameterUI(
	PF_InData			*in_data,
	PF_OutData			*out_data,
	PF_ParamDef			*params[],
	PF_LayerDef			*outputP)
{
	PF_Err				err = PF_Err_NONE;
	AEGP_SuiteHandler	suites(in_data->pica_basicP);

	PF_ParamDef		param_copy[RMFOURIER_NUM_PARAMS];
	ERR(MakeParamCopy(params, param_copy));

	return err;
}*/

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
	PF_ParamDef inverseFftParam, phaseParam, colorMode;
	PF_RenderRequest req = extra->input->output_request;
	PF_CheckoutResult in_result, phase_result;

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
				RMFOURIER_INVERSE_FFT,
				in_data->current_time,
				in_data->time_step,
				in_data->time_scale,
				&inverseFftParam
			));

			AEFX_CLR_STRUCT(phaseParam);
			ERR(PF_CHECKOUT_PARAM(
				in_data,
				RMFOURIER_FFT_PHASE,
				in_data->current_time,
				in_data->time_step,
				in_data->time_scale,
				&phaseParam
			));

			AEFX_CLR_STRUCT(colorMode);
			ERR(PF_CHECKOUT_PARAM(
				in_data,
				RMFOURIER_COLOR_MODE,
				in_data->current_time,
				in_data->time_step,
				in_data->time_scale,
				&colorMode
			));

			if (!err) {
				req.preserve_rgb_of_zero_alpha = FALSE;	
				req.field = PF_Field_FRAME;				

				// Checkout layers
				ERR(extra->cb->checkout_layer(  in_data->effect_ref,
												RMFOURIER_INPUT,
												RMFOURIER_INPUT,
												&req,
												in_data->current_time,
												in_data->time_step,
												in_data->time_scale,
												&in_result ));

				ERR(extra->cb->checkout_layer(  in_data->effect_ref,
												RMFOURIER_PHASE_LAYER,
												RMFOURIER_PHASE_LAYER,
												&req,
												in_data->current_time,
												in_data->time_step,
												in_data->time_scale,
												&phase_result));

				if (!err) {
					AEFX_CLR_STRUCT(*infoP);
					// Here you get the input values
					infoP->inverseCB 	= inverseFftParam.u.bd.value;
					infoP->fftPhase = phaseParam.u.bd.value;

					if (colorMode.u.pd.value == 5) infoP->colorComputations[3] = true; // Grayscale
					else {
						if (colorMode.u.pd.value == 1) { // RGB
							infoP->colorComputations[0] = true; // R
							infoP->colorComputations[1] = true; // G
							infoP->colorComputations[2] = true; // B
						}
						else {
							if (colorMode.u.pd.value == 2) infoP->colorComputations[0] = true; // R
							if (colorMode.u.pd.value == 3) infoP->colorComputations[1] = true; // G
							if (colorMode.u.pd.value == 4) infoP->colorComputations[2] = true; // B
						}
					}

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
	PF_EffectWorld		*input_worldP = nullptr,
						*output_worldP = nullptr,
						*phase_worldP = nullptr,
						copy_worldP;
	PF_WorldSuite2		*wsP = NULL;
	PF_PixelFormat		format = PF_PixelFormat_INVALID;
	PF_ParamDef			phaseLayerParam, normParam;
	bool				letsRender = true;

	rmFourierInfo	*infoP = reinterpret_cast<rmFourierInfo*>(suites.HandleSuite1()->host_lock_handle(reinterpret_cast<PF_Handle>(extra->input->pre_render_data)));

	if (infoP) {
		if (!infoP->no_opB) {
			// checkout input & output buffers.
			ERR((extra->cb->checkout_layer_pixels(in_data->effect_ref, RMFOURIER_INPUT, &input_worldP)));
			ERR((extra->cb->checkout_layer_pixels(in_data->effect_ref, RMFOURIER_PHASE_LAYER, &phase_worldP)));
			ERR(extra->cb->checkout_output(in_data->effect_ref, &output_worldP));

			/*A_long numOfLayers = NULL;
			AEGP_LayerH  activeLayer;
			AEGP_MaskRefH mask01;

			// Check mask
			ERR(suites.LayerSuite8()->AEGP_GetActiveLayer(&activeLayer));
			ERR(suites.MaskSuite6()->AEGP_GetLayerNumMasks(activeLayer, &numOfLayers));
			ERR(suites.MaskSuite6()->AEGP_GetLayerMaskByIndex(activeLayer, 0, &mask01));
			ERR(suites.MaskSuite6()->AEGP_DisposeMask(mask01));*/

			if (!err && output_worldP) {
				ERR(AEFX_AcquireSuite(in_data,
					out_data,
					kPFWorldSuite,
					kPFWorldSuiteVersion2,
					"Couldn't load suite.",
					(void**)&wsP));

				// Checkout params
				AEFX_CLR_STRUCT(phaseLayerParam);
				ERR(PF_CHECKOUT_PARAM(
					in_data,
					RMFOURIER_PHASE_LAYER,
					in_data->current_time,//(in_data->current_time + params[CHECK_FRAME]->u.sd.value * in_data->time_step),
					in_data->time_step,
					in_data->time_scale,
					&phaseLayerParam));

				AEFX_CLR_STRUCT(normParam);
				ERR(PF_CHECKOUT_PARAM(
					in_data,
					RMFOURIER_NORMALIZE,
					in_data->current_time,
					in_data->time_step,
					in_data->time_scale,
					&normParam
				));

				infoP->ref = in_data->effect_ref;
				infoP->samp_pb.src = input_worldP;
				infoP->in_data = *in_data;
				infoP->inD = in_data;
				infoP->input_worldP = input_worldP;
				infoP->output_worldP = output_worldP;
				infoP->normalizeVal = normParam.u.fs_d.value;

				// Checkin params
				ERR2(PF_CHECKIN_PARAM(in_data, &phaseLayerParam));

				ERR(wsP->PF_GetPixelFormat(input_worldP, &format));

				if (infoP->inverseCB) {
					if (!phase_worldP) letsRender = false;
					else {
						if (!((phase_worldP->width == input_worldP->width) && (phase_worldP->height == input_worldP->height))) {
							letsRender = false;

							auto ansiSuite = AEFX_SuiteScoper<PF_ANSICallbacksSuite1>(in_data, kPFANSISuite, kPFANSISuiteVersion1);
							ansiSuite->sprintf(out_data->return_msg, "The phase layer have to be the same size as the input layer.");
						}
					}
				}
					

				if (letsRender){
					if (!err) {

						infoP->currentProcess = 0;
						infoP->expTable.clear();
						infoP->convExpTable.clear();

						infoP->inWidth = input_worldP->width; //in_data->width;
						infoP->inHeight = input_worldP->height;//in_data->height;
						infoP->outWidth = out_data->width;
						infoP->outHeight = out_data->height;

						A_long imgSize = infoP->inWidth * infoP->inHeight;

						fftw_plan planR, planG, planB, planGS;
						//int a = fftw_init_threads();

						// Initialize the vectors
						infoP->preR = new double[infoP->inWidth * infoP->inHeight];
						infoP->preG = new double[infoP->inWidth * infoP->inHeight];
						infoP->preB = new double[infoP->inWidth * infoP->inHeight];
						if (infoP->colorComputations[3]) {
							infoP->inVectorGS = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * infoP->inWidth * infoP->inHeight);
							infoP->outVectorGS = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * infoP->inWidth * infoP->inHeight);
						}
						else {
							if (infoP->colorComputations[0]) {
								infoP->inVectorR = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * infoP->inWidth * infoP->inHeight);
								infoP->outVectorR = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * infoP->inWidth * infoP->inHeight);
							}
							if (infoP->colorComputations[1]) {
								infoP->inVectorG = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * infoP->inWidth * infoP->inHeight);
								infoP->outVectorG = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * infoP->inWidth * infoP->inHeight);
							}
							if (infoP->colorComputations[2]) {
								infoP->inVectorB = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * infoP->inWidth * infoP->inHeight);
								infoP->outVectorB = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * infoP->inWidth * infoP->inHeight);
							}
						}

						int sign;
						if (infoP->inverseCB) sign = FFTW_BACKWARD;
						else sign = FFTW_FORWARD;

						switch (format) {

							case PF_PixelFormat_ARGB128: {
								if (input_worldP->data && output_worldP->data) {

									clock_t start, end;
									double time_taken;
									start = clock();

									//ERR(PF_PROGRESS(in_data, 1, 100));

									if (infoP->inverseCB) {
										infoP->tmp_worldP = phase_worldP;

										// Compute the fftshift
										ERR(suites.IterateSuite1()->AEGP_IterateGeneric(
											infoP->inHeight,
											(void*)infoP,
											ifftShift));
									}

									// Get the pixels from worldspace and fill the tmp vectors with it.
									ERR(suites.IterateSuite1()->AEGP_IterateGeneric(
										infoP->inHeight,
										(void*)infoP,
										pixelToVector));

									// Get max number of threads
									//ERR(suites.IterateSuite1()->AEGP_GetNumThreads(&infoP->nMaxThreads));

									//fftw_plan_with_nthreads(infoP->nMaxThreads);

									// Compute FFT or IFFT
									if (infoP->colorComputations[3]) {
										planGS = fftw_plan_dft_2d(infoP->inHeight, infoP->inWidth, infoP->inVectorGS, infoP->outVectorGS, sign, FFTW_ESTIMATE);
										fftw_execute(planGS);
									}
									else {
										if (infoP->colorComputations[0]) {
											planR = fftw_plan_dft_2d(infoP->inHeight, infoP->inWidth, infoP->inVectorR, infoP->outVectorR, sign, FFTW_ESTIMATE);
											fftw_execute(planR);
										}
										if (infoP->colorComputations[1]) {
											planG = fftw_plan_dft_2d(infoP->inHeight, infoP->inWidth, infoP->inVectorG, infoP->outVectorG, sign, FFTW_ESTIMATE);
											fftw_execute(planG);
										}
										if (infoP->colorComputations[2]) {
											planB = fftw_plan_dft_2d(infoP->inHeight, infoP->inWidth, infoP->inVectorB, infoP->outVectorB, sign, FFTW_ESTIMATE);
											fftw_execute(planB);
										}
									}
									
									// Calculate Magnitudes, Phases, and Maximun values
									ERR(suites.IterateFloatSuite1()->iterate(
										in_data,
										0,							// progress base
										infoP->inHeight,			// progress final
										input_worldP,				// src
										NULL,						// area - null for all pixels
										(void*)infoP,				// custom data pointer
										preVectorToPixel,				// pixel function pointer
										output_worldP
									));

									// Put the values from the vector back to the worldspace
									ERR(suites.IterateFloatSuite1()->iterate(
										in_data,
										0,							// progress base
										infoP->inHeight,			// progress final
										input_worldP,				// src
										NULL,						// area - null for all pixels
										(void*)infoP,				// custom data pointer
										vectorToPixel,				// pixel function pointer
										output_worldP
									));

									// Free memory
									delete[] infoP->preR;
									delete[] infoP->preG;
									delete[] infoP->preB;
									if (infoP->colorComputations[3]) {
										fftw_destroy_plan(planGS);
										fftw_free(infoP->inVectorGS);
										fftw_free(infoP->outVectorGS);
									}
									else {
										if (infoP->colorComputations[0]) {
											fftw_destroy_plan(planR);
											fftw_free(infoP->inVectorR);
											fftw_free(infoP->outVectorR);
										}
										if (infoP->colorComputations[1]) {
											fftw_destroy_plan(planG);
											fftw_free(infoP->inVectorG);
											fftw_free(infoP->outVectorG);
										}
										if (infoP->colorComputations[2]) {
											fftw_destroy_plan(planB);
											fftw_free(infoP->inVectorB);
											fftw_free(infoP->outVectorB);
										}
									}

									end = clock();
									time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;

									//fftw_cleanup_threads();

									// If I want to get the magnitude only
									if (!infoP->inverseCB && !infoP->fftPhase) {
										// Normalize the image
										/*ERR(suites.IterateFloatSuite1()->iterate(
											in_data,
											0,							// progress base
											output_worldP->height,		// progress final
											output_worldP,				// src
											NULL,						// area - null for all pixels
											(void*)infoP,				// custom data pointer
											normalizeImg,				// pixel function pointer
											output_worldP
										));*/

										// Before I do the fftshift I need to make a copy of 
										// the output world so I can compute the effect in
										// multithread mode. (tmp solution. I might change this in the future)
										ERR(wsP->PF_NewWorld(in_data->effect_ref,	// New world
											infoP->inWidth,
											infoP->inHeight,
											NULL,
											PF_PixelFormat_ARGB128,
											&copy_worldP));

										ERR(suites.WorldTransformSuite1()->copy(	// Copy the output data to the new world
											in_data->effect_ref,
											output_worldP,
											&copy_worldP,
											NULL,
											NULL));

										infoP->copy_worldP = &copy_worldP;	// Get the pointer

										// Compute the fftshift
										ERR(suites.IterateSuite1()->AEGP_IterateGeneric(
										infoP->inHeight,
										(void*)infoP,
										fftShift));

										// Free memory
										ERR(wsP->PF_DisposeWorld(in_data->effect_ref, &copy_worldP));
									}
									
									if (infoP->inverseCB) {
										// Normalize the image
										ERR(suites.IterateFloatSuite1()->iterate(
											in_data,
											0,							// progress base
											infoP->inHeight,			// progress final
											output_worldP,				// src
											NULL,						// area - null for all pixels
											(void*)infoP,				// custom data pointer
											normalizeImg,				// pixel function pointer
											output_worldP
										));
									}
								}

								break;
							}
						

							case PF_PixelFormat_ARGB64: {
								infoP->inWidth = input_worldP->width; //in_data->width;
								infoP->inHeight = input_worldP->height;//in_data->height;

																	   // Circular shift
								ERR(suites.Iterate16Suite1()->iterate(// TODO: to perfoemr faster give an area
									in_data,
									0,							// progress base
									output_worldP->height,		// progress final
									output_worldP,				// src
									NULL,						// area - null for all pixels
									(void*)infoP,				// custom data pointer
									tmpRender16,				// pixel function pointer
									output_worldP
								));

								break;
							}
						

							case PF_PixelFormat_ARGB32: {
								infoP->inWidth = input_worldP->width; //in_data->width;
								infoP->inHeight = input_worldP->height;//in_data->height;
								
								ERR(suites.Iterate8Suite1()->iterate(
									in_data,
									0,							// progress base
									output_worldP->height,		// progress final
									input_worldP,				// src
									NULL,						// area - null for all pixels
									(void*)infoP,				// custom data pointer
									tmpRender8,				// pixel function pointer
									output_worldP
								));

								break;
							}
						

							default:
								err = PF_Err_BAD_CALLBACK_PARAM;
								break;
						}
					}
				}
				else {
					// copy input buffer;
					ERR(PF_COPY(input_worldP, output_worldP, NULL, NULL));
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

	ERR2(PF_CHECKIN_PARAM(in_data, &normParam)); // THIS MIGHT GENERATE BUGS ?

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

			case PF_Cmd_USER_CHANGED_PARAM:
				err = UserChangedParam(in_data,
					out_data,
					params,
					output,
					reinterpret_cast<const PF_UserChangedParamExtra *>(extra));
				break;

			/*case PF_Cmd_UPDATE_PARAMS_UI:
				err = UpdateParameterUI(in_data,
					out_data,
					params,
					output);
				break;*/
		}
	}
	catch(PF_Err &thrown_err){
		err = thrown_err;
	}
	return err;
}

