/********************************************************************
 * This file includes functions to build bitstream database
 *******************************************************************/
/* Headers from vtrutil library */
#include "vtr_time.h"
#include "vtr_log.h"

/* Headers from openfpgashell library */
#include "command_exit_codes.h"

/* Headers from openfpgautil library */
#include "openfpga_digest.h"

/* Headers from fpgabitstream library */
#include "read_xml_arch_bitstream.h"
#include "write_xml_arch_bitstream.h"
#include "report_arch_bitstream_distribution.h"

#include "openfpga_naming.h"

#include "build_device_bitstream.h"
#include "write_text_fabric_bitstream.h"
#include "write_xml_fabric_bitstream.h"
#include "build_fabric_bitstream.h"
#include "build_io_mapping_info.h"
#include "write_xml_io_mapping.h"
#include "openfpga_bitstream.h"

/* Include global variables of VPR */
#include "globals.h"

/* begin namespace openfpga */
namespace openfpga {

/********************************************************************
 * A wrapper function to call the build_device_bitstream() in FPGA bitstream
 *******************************************************************/
int fpga_bitstream(OpenfpgaContext& openfpga_ctx,
                   const Command& cmd, const CommandContext& cmd_context) {

  CommandOptionId opt_verbose = cmd.option("verbose");
  CommandOptionId opt_write_file = cmd.option("write_file");
  CommandOptionId opt_read_file = cmd.option("read_file");

  if (true == cmd_context.option_enable(cmd, opt_read_file)) {
    openfpga_ctx.mutable_bitstream_manager() = read_xml_architecture_bitstream(cmd_context.option_value(cmd, opt_read_file).c_str());
  } else {
    openfpga_ctx.mutable_bitstream_manager() = build_device_bitstream(g_vpr_ctx,
                                                                      openfpga_ctx,
                                                                      cmd_context.option_enable(cmd, opt_verbose));
  }

  if (true == cmd_context.option_enable(cmd, opt_write_file)) {
    std::string src_dir_path = find_path_dir_name(cmd_context.option_value(cmd, opt_write_file));

    /* Create directories */
    create_directory(src_dir_path);

    write_xml_architecture_bitstream(openfpga_ctx.bitstream_manager(),
                                     cmd_context.option_value(cmd, opt_write_file));
  }

  /* TODO: should identify the error code from internal function execution */
  return CMD_EXEC_SUCCESS;
}

/********************************************************************
 * A wrapper function to call the build_fabric_bitstream() in FPGA bitstream
 *******************************************************************/
int build_fabric_bitstream(OpenfpgaContext& openfpga_ctx,
                           const Command& cmd, const CommandContext& cmd_context) {

  CommandOptionId opt_verbose = cmd.option("verbose");

  /* Build fabric bitstream here */
  openfpga_ctx.mutable_fabric_bitstream() = build_fabric_dependent_bitstream(openfpga_ctx.bitstream_manager(),
                                                                             openfpga_ctx.module_graph(),
                                                                             openfpga_ctx.arch().config_protocol,
                                                                             cmd_context.option_enable(cmd, opt_verbose));

  /* TODO: should identify the error code from internal function execution */
  return CMD_EXEC_SUCCESS;
}

/********************************************************************
 * A wrapper function to call the write_fabric_bitstream() in FPGA bitstream
 *******************************************************************/
int write_fabric_bitstream(const OpenfpgaContext& openfpga_ctx,
                           const Command& cmd, const CommandContext& cmd_context) {

  CommandOptionId opt_verbose = cmd.option("verbose");
  CommandOptionId opt_file = cmd.option("file");
  CommandOptionId opt_file_format = cmd.option("format");

  /* Write fabric bitstream if required */
  int status = CMD_EXEC_SUCCESS;
  
  VTR_ASSERT(true == cmd_context.option_enable(cmd, opt_file));

  std::string src_dir_path = find_path_dir_name(cmd_context.option_value(cmd, opt_file));

  /* Create directories */
  create_directory(src_dir_path);

  /* Check file format requirements */
  std::string file_format("plain_text"); 
  if (true == cmd_context.option_enable(cmd, opt_file_format)) {
    file_format = cmd_context.option_value(cmd, opt_file_format);
  }

  if (std::string("xml") == file_format) {
    status = write_fabric_bitstream_to_xml_file(openfpga_ctx.bitstream_manager(),
                                                openfpga_ctx.fabric_bitstream(),
                                                openfpga_ctx.arch().config_protocol,
                                                cmd_context.option_value(cmd, opt_file),
                                                cmd_context.option_enable(cmd, opt_verbose));
  } else {
    /* By default, output in plain text format */
    status = write_fabric_bitstream_to_text_file(openfpga_ctx.bitstream_manager(),
                                                 openfpga_ctx.fabric_bitstream(),
                                                 openfpga_ctx.arch().config_protocol,
                                                 cmd_context.option_value(cmd, opt_file),
                                                 cmd_context.option_enable(cmd, opt_verbose));
  }
  
  return status;
} 

/********************************************************************
 * A wrapper function to call the write_io_mapping() in FPGA bitstream
 *******************************************************************/
int write_io_mapping(const OpenfpgaContext& openfpga_ctx,
                     const Command& cmd, const CommandContext& cmd_context) {

  CommandOptionId opt_verbose = cmd.option("verbose");
  CommandOptionId opt_file = cmd.option("file");

  /* Write fabric bitstream if required */
  int status = CMD_EXEC_SUCCESS;
  
  VTR_ASSERT(true == cmd_context.option_enable(cmd, opt_file));

  std::string src_dir_path = find_path_dir_name(cmd_context.option_value(cmd, opt_file));

  /* Create directories */
  create_directory(src_dir_path);

  /* Create a module as the top-level fabric, and add it to the module manager */
  std::string top_module_name = generate_fpga_top_module_name();
  ModuleId top_module = openfpga_ctx.module_graph().find_module(top_module_name);
  VTR_ASSERT(true == openfpga_ctx.module_graph().valid_module_id(top_module));

  IoMap io_map = build_fpga_io_mapping_info(openfpga_ctx.module_graph(),
                                            top_module,
                                            g_vpr_ctx.atom(),
                                            g_vpr_ctx.placement(),
                                            openfpga_ctx.io_location_map(),
                                            openfpga_ctx.vpr_netlist_annotation(),
                                            std::string(),
                                            std::string());

  status = write_io_mapping_to_xml_file(io_map,
                                        cmd_context.option_value(cmd, opt_file),
                                        cmd_context.option_enable(cmd, opt_verbose));
  
  return status;
} 

/********************************************************************
 * A wrapper function to call the report_arch_bitstream_distribution() in FPGA bitstream
 *******************************************************************/
int report_bitstream_distribution(const OpenfpgaContext& openfpga_ctx,
                                  const Command& cmd, const CommandContext& cmd_context) {

  CommandOptionId opt_file = cmd.option("file");

  int status = CMD_EXEC_SUCCESS;
  
  VTR_ASSERT(true == cmd_context.option_enable(cmd, opt_file));

  std::string src_dir_path = find_path_dir_name(cmd_context.option_value(cmd, opt_file));

  /* Create directories */
  create_directory(src_dir_path);

  /* Default depth requirement, this is to limit the report size by default */
  int depth = 1;
  CommandOptionId opt_depth = cmd.option("depth");
  if (true == cmd_context.option_enable(cmd, opt_depth)) {
    depth = std::atoi(cmd_context.option_value(cmd, opt_depth).c_str());
    /* Error out if we have negative depth */
    if (0 > depth) {
      VTR_LOG_ERROR("Invalid depth '%d' which should be 0 or a positive number!\n",
                    depth);
      return CMD_EXEC_FATAL_ERROR; 
    }
  }

  status = report_architecture_bitstream_distribution(openfpga_ctx.bitstream_manager(),
                                                      cmd_context.option_value(cmd, opt_file),
                                                      depth);
  
  return status;
}

} /* end namespace openfpga */
