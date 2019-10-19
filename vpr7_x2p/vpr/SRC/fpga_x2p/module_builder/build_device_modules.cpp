/********************************************************************
 * This file includes the main function to build module graphs
 * for the FPGA fabric
 *******************************************************************/
#include <time.h>
#include <unistd.h>

#include "vtr_assert.h"
#include "util.h"
#include "spice_types.h"
#include "fpga_x2p_utils.h"

#include "build_device_modules.h"

/********************************************************************
 * The main function to be called for building module graphs 
 * for a FPGA fabric
 *******************************************************************/
ModuleManager build_device_module_graph(const t_vpr_setup& vpr_setup,
                                        const t_arch& arch,
                                        const MuxLibrary& mux_lib) {
  /* Check if the routing architecture we support*/
  if (UNI_DIRECTIONAL != vpr_setup.RoutingArch.directionality) {
    vpr_printf(TIO_MESSAGE_ERROR, 
               "FPGA X2P only supports uni-directional routing architecture!\n");
    exit(1);
  }

  /* We don't support mrFPGA */
#ifdef MRFPGA_H
  if (is_mrFPGA) {
    vpr_printf(TIO_MESSAGE_ERROR, 
               "FPGA X2P does not support mrFPGA!\n");
    exit(1);
  }
#endif

  /* Module Graph builder formally starts*/
  vpr_printf(TIO_MESSAGE_INFO, "\nStart building module graphs for FPGA fabric...\n");

  /* Module manager to be built */
  ModuleManager module_manager;

  /* Start time count */
  clock_t t_start = clock();

  /* Assign the SRAM model applied to the FPGA fabric */
  VTR_ASSERT(NULL != arch.sram_inf.verilog_sram_inf_orgz); /* Check !*/
  t_spice_model* mem_model = arch.sram_inf.verilog_sram_inf_orgz->spice_model;
  /* initialize the SRAM organization information struct */
  CircuitModelId sram_model = arch.spice->circuit_lib.model(mem_model->name);  
  VTR_ASSERT(CircuitModelId::INVALID() != sram_model);

  /* TODO: This should be moved to FPGA-X2P setup 
   * Check all the SRAM port is using the correct SRAM circuit model 
   */
  config_spice_models_sram_port_spice_model(arch.spice->num_spice_model, 
                                            arch.spice->spice_models,
                                            arch.sram_inf.verilog_sram_inf_orgz->spice_model);
  config_circuit_models_sram_port_to_default_sram_model(arch.spice->circuit_lib, arch.sram_inf.verilog_sram_inf_orgz->circuit_model); 

  /* TODO: Build elmentary modules */


  /* End time count */
  clock_t t_end = clock();

  float run_time_sec = (float)(t_end - t_start) / CLOCKS_PER_SEC;
  vpr_printf(TIO_MESSAGE_INFO, "Building module graphs took %g seconds\n", run_time_sec);  


  return module_manager;
}