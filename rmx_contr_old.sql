-- phpMyAdmin SQL Dump
-- version 4.0.10deb1
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: Apr 20, 2018 at 10:53 AM
-- Server version: 5.6.33-0ubuntu0.14.04.1
-- PHP Version: 5.5.9-1ubuntu4.22

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `rmx_contr`
--

-- --------------------------------------------------------

--
-- Table structure for table `active_service_list`
--

CREATE TABLE IF NOT EXISTS `active_service_list` (
  `in_channel` int(2) NOT NULL,
  `out_channel` int(2) NOT NULL,
  `channel_num` int(10) NOT NULL,
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`rmx_id`,`in_channel`,`out_channel`,`channel_num`),
  UNIQUE KEY `ioc` (`in_channel`,`out_channel`,`channel_num`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `alarm_flags`
--

CREATE TABLE IF NOT EXISTS `alarm_flags` (
  `FIFO_Threshold0` int(11) NOT NULL,
  `FIFO_Threshold1` int(11) NOT NULL,
  `FIFO_Threshold2` int(11) NOT NULL,
  `FIFO_Threshold3` int(11) NOT NULL,
  `mode` int(1) NOT NULL,
  `rmx_id` int(1) NOT NULL,
  PRIMARY KEY (`rmx_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `channel_list`
--

CREATE TABLE IF NOT EXISTS `channel_list` (
  `input_channel` int(2) NOT NULL,
  `channel_number` int(11) NOT NULL,
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`rmx_id`,`input_channel`,`channel_number`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `config_allegro`
--

CREATE TABLE IF NOT EXISTS `config_allegro` (
  `mxl_id` int(11) NOT NULL,
  `address` int(11) NOT NULL,
  `enable1` int(11) NOT NULL,
  `volt1` int(11) NOT NULL,
  `enable2` int(11) NOT NULL,
  `volt2` int(11) NOT NULL,
  PRIMARY KEY (`mxl_id`,`address`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `config_allegro`
--

INSERT INTO `config_allegro` (`mxl_id`, `address`, `enable1`, `volt1`, `enable2`, `volt2`) VALUES
(1, 8, 1, 5, 1, 5),
(1, 9, 1, 5, 1, 5),
(2, 8, 1, 5, 1, 5),
(2, 9, 1, 5, 1, 5),
(3, 8, 1, 5, 1, 5),
(5, 8, 1, 5, 1, 5),
(6, 8, 1, 5, 1, 5);

-- --------------------------------------------------------

--
-- Table structure for table `create_alarm_flags`
--

CREATE TABLE IF NOT EXISTS `create_alarm_flags` (
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  `level1` int(11) NOT NULL,
  `level2` int(11) NOT NULL,
  `level3` int(11) NOT NULL,
  `level4` int(11) NOT NULL,
  `mode` int(11) NOT NULL,
  PRIMARY KEY (`rmx_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `custom_pids`
--

CREATE TABLE IF NOT EXISTS `custom_pids` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `rmx_no` int(1) NOT NULL,
  `output_channel` int(1) NOT NULL,
  `pid` varchar(200) NOT NULL,
  `output_auth` int(10) NOT NULL DEFAULT '255',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=latin1 AUTO_INCREMENT=5 ;

-- --------------------------------------------------------

--
-- Table structure for table `dvb_spi_output`
--

CREATE TABLE IF NOT EXISTS `dvb_spi_output` (
  `output` int(1) NOT NULL,
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  `bit_rate` int(15) NOT NULL,
  `falling` int(1) NOT NULL,
  `mode` int(1) NOT NULL,
  PRIMARY KEY (`rmx_id`,`output`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `ecmg`
--

CREATE TABLE IF NOT EXISTS `ecmg` (
  `channel_id` int(10) NOT NULL,
  `supercas_id` varchar(10) NOT NULL,
  `ip` varchar(20) NOT NULL,
  `port` int(10) NOT NULL,
  `is_enable` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`channel_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `ecm_stream`
--

CREATE TABLE IF NOT EXISTS `ecm_stream` (
  `stream_id` int(10) NOT NULL,
  `channel_id` int(10) NOT NULL,
  `ecm_id` int(10) NOT NULL,
  `access_criteria` varchar(10) NOT NULL,
  `cp_number` int(10) NOT NULL,
  `timestamp` varchar(10) NOT NULL,
  PRIMARY KEY (`stream_id`,`channel_id`),
  KEY `channel_id` (`channel_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `emmg`
--

CREATE TABLE IF NOT EXISTS `emmg` (
  `channel_id` int(10) NOT NULL,
  `client_id` varchar(10) NOT NULL,
  `data_id` int(10) NOT NULL,
  `bw` int(10) NOT NULL,
  `port` int(10) NOT NULL,
  `stream_id` int(10) NOT NULL,
  `is_enable` int(2) NOT NULL DEFAULT '1',
  `emm_pid` int(11) NOT NULL,
  PRIMARY KEY (`channel_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `emmg_descriptor`
--

CREATE TABLE IF NOT EXISTS `emmg_descriptor` (
  `channel_id` int(11) NOT NULL,
  `rmx_no` int(11) NOT NULL,
  `output` int(11) NOT NULL,
  `is_enable` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`channel_id`,`rmx_no`,`output`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `encrypted_service_list`
--

CREATE TABLE IF NOT EXISTS `encrypted_service_list` (
  `channel_num` int(11) NOT NULL,
  `in_channel` int(11) NOT NULL,
  `out_channel` int(11) NOT NULL,
  `rmx_id` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Firmware`
--

CREATE TABLE IF NOT EXISTS `Firmware` (
  `rmx_id` int(1) NOT NULL,
  `major_ver` int(11) NOT NULL,
  `min_ver` int(11) NOT NULL,
  `standard` int(11) NOT NULL,
  `cust_option_id` int(11) NOT NULL,
  `cust_option_ver` int(11) NOT NULL,
  PRIMARY KEY (`rmx_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `Firmware`
--

INSERT INTO `Firmware` (`rmx_id`, `major_ver`, `min_ver`, `standard`, `cust_option_id`, `cust_option_ver`) VALUES
(0, 13, 8, 0, 0, 0);

-- --------------------------------------------------------

--
-- Table structure for table `freeCA_mode_programs`
--

CREATE TABLE IF NOT EXISTS `freeCA_mode_programs` (
  `program_number` int(11) NOT NULL,
  `input_channel` int(2) NOT NULL,
  `output_channel` int(2) NOT NULL,
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`program_number`,`input_channel`,`output_channel`,`rmx_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `highPriorityServices`
--

CREATE TABLE IF NOT EXISTS `highPriorityServices` (
  `program_number` int(11) NOT NULL,
  `input` int(11) NOT NULL,
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`rmx_id`,`input`,`program_number`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `HWversion`
--

CREATE TABLE IF NOT EXISTS `HWversion` (
  `rmx_id` int(1) NOT NULL,
  `major_ver` int(11) NOT NULL,
  `min_ver` int(11) NOT NULL,
  `no_of_input` int(11) NOT NULL,
  `no_of_output` int(11) NOT NULL,
  `fifo_size` int(11) NOT NULL,
  `options_implemented` int(11) NOT NULL,
  `core_clk` double NOT NULL,
  PRIMARY KEY (`rmx_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `HWversion`
--

INSERT INTO `HWversion` (`rmx_id`, `major_ver`, `min_ver`, `no_of_input`, `no_of_output`, `fifo_size`, `options_implemented`, `core_clk`) VALUES
(0, 10, 14, 4, 4, 85, 0, 200);

-- --------------------------------------------------------

--
-- Table structure for table `Ifrequency`
--

CREATE TABLE IF NOT EXISTS `Ifrequency` (
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  `ifrequency` int(10) NOT NULL,
  PRIMARY KEY (`rmx_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `Ifrequency`
--

INSERT INTO `Ifrequency` (`rmx_id`, `ifrequency`) VALUES
(1, 142),
(2, 206),
(3, 270),
(4, 334),
(5, 398),
(6, 462);

-- --------------------------------------------------------

--
-- Table structure for table `input`
--

CREATE TABLE IF NOT EXISTS `input` (
  `input_channel` int(2) NOT NULL,
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  `tuner_type` int(1) NOT NULL DEFAULT '0',
  PRIMARY KEY (`input_channel`,`rmx_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `input`
--

INSERT INTO `input` (`input_channel`, `rmx_id`, `tuner_type`) VALUES
(0, 1, 0),
(0, 2, 0),
(0, 3, 0),
(0, 4, 0),
(0, 5, 0),
(0, 6, 0),
(1, 1, 0),
(1, 2, 0),
(1, 3, 0),
(1, 4, 0),
(1, 5, 0),
(1, 6, 0),
(2, 1, 0),
(2, 2, 0),
(2, 3, 0),
(2, 4, 0),
(2, 5, 0),
(2, 6, 0),
(3, 1, 0),
(3, 2, 0),
(3, 3, 0),
(3, 4, 0),
(3, 5, 0),
(3, 6, 0),
(4, 1, 0),
(4, 2, 0),
(4, 3, 0),
(4, 4, 0),
(4, 5, 0),
(4, 6, 0),
(5, 1, 0),
(5, 2, 0),
(5, 3, 0),
(5, 4, 0),
(5, 5, 0),
(5, 6, 0),
(6, 1, 0),
(6, 2, 0),
(6, 3, 0),
(6, 4, 0),
(6, 5, 0),
(6, 6, 0),
(7, 1, 0),
(7, 2, 0),
(7, 3, 0),
(7, 4, 0),
(7, 5, 0),
(7, 6, 0);

-- --------------------------------------------------------

--
-- Table structure for table `input_mode`
--

CREATE TABLE IF NOT EXISTS `input_mode` (
  `input` int(1) NOT NULL,
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  `is_spts` int(1) NOT NULL,
  `pmt` int(4) NOT NULL,
  `sid` int(6) NOT NULL,
  `rise` int(1) NOT NULL,
  `master` int(1) NOT NULL,
  `in_select` int(1) NOT NULL,
  `bitrate` int(10) NOT NULL,
  PRIMARY KEY (`input`,`rmx_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `ip_input_channels`
--

CREATE TABLE IF NOT EXISTS `ip_input_channels` (
  `rmx_no` int(1) NOT NULL,
  `input_channel` int(1) NOT NULL,
  `ip_address` varchar(20) NOT NULL,
  `port` int(10) NOT NULL,
  `type` int(1) NOT NULL DEFAULT '1',
  `is_enable` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`rmx_no`,`input_channel`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `ip_output_channels`
--

CREATE TABLE IF NOT EXISTS `ip_output_channels` (
  `rmx_no` int(1) NOT NULL,
  `output_channel` int(1) NOT NULL,
  `ip_address` varchar(20) NOT NULL,
  `port` int(10) NOT NULL,
  `is_enable` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`rmx_no`,`output_channel`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `lcn`
--

CREATE TABLE IF NOT EXISTS `lcn` (
  `program_number` int(10) NOT NULL,
  `input` int(11) NOT NULL,
  `channel_number` int(10) NOT NULL,
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`rmx_id`,`input`,`program_number`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `lcn_provider_id`
--

CREATE TABLE IF NOT EXISTS `lcn_provider_id` (
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  `provider_id` int(11) NOT NULL,
  PRIMARY KEY (`rmx_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `locked_programs`
--

CREATE TABLE IF NOT EXISTS `locked_programs` (
  `program_number` int(11) NOT NULL,
  `input` int(10) NOT NULL,
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`rmx_id`,`input`,`program_number`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `mux_15_1`
--

CREATE TABLE IF NOT EXISTS `mux_15_1` (
  `mux_id` int(1) NOT NULL,
  `is_enable` int(1) NOT NULL DEFAULT '0',
  PRIMARY KEY (`mux_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `mux_15_1`
--

INSERT INTO `mux_15_1` (`mux_id`, `is_enable`) VALUES
(1, 0),
(2, 0),
(3, 0);

-- --------------------------------------------------------

--
-- Table structure for table `network_details`
--

CREATE TABLE IF NOT EXISTS `network_details` (
  `output` int(1) NOT NULL,
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  `ts_id` int(10) NOT NULL DEFAULT '-1',
  `network_id` int(10) NOT NULL DEFAULT '-1',
  `original_netw_id` int(10) NOT NULL DEFAULT '-1',
  `network_name` varchar(30) NOT NULL DEFAULT '-1',
  PRIMARY KEY (`output`,`rmx_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `new_service_namelist`
--

CREATE TABLE IF NOT EXISTS `new_service_namelist` (
  `channel_number` int(10) NOT NULL,
  `channel_name` varchar(40) DEFAULT '-1',
  `service_id` int(10) NOT NULL DEFAULT '-1',
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`channel_number`,`rmx_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `NIT_table_timeout`
--

CREATE TABLE IF NOT EXISTS `NIT_table_timeout` (
  `timeout` int(11) NOT NULL,
  `table_id` int(11) NOT NULL,
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`rmx_id`,`table_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `output`
--

CREATE TABLE IF NOT EXISTS `output` (
  `output_channel` int(2) NOT NULL,
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  `qam_id` int(1) NOT NULL DEFAULT '2',
  `nit_mode` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`output_channel`,`rmx_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `output`
--

INSERT INTO `output` (`output_channel`, `rmx_id`, `qam_id`, `nit_mode`) VALUES
(0, 1, 2, 1),
(0, 2, 2, 1),
(0, 3, 2, 1),
(0, 4, 2, 1),
(0, 5, 2, 1),
(0, 6, 2, 1),
(1, 1, 2, 1),
(1, 2, 2, 1),
(1, 3, 2, 1),
(1, 4, 2, 1),
(1, 5, 2, 1),
(1, 6, 2, 1),
(2, 1, 2, 1),
(2, 2, 2, 1),
(2, 3, 2, 1),
(2, 4, 2, 1),
(2, 5, 2, 1),
(2, 6, 2, 1),
(3, 1, 2, 1),
(3, 2, 2, 1),
(3, 3, 2, 1),
(3, 4, 2, 1),
(3, 5, 2, 1),
(3, 6, 2, 1),
(4, 1, 2, 1),
(4, 2, 2, 1),
(4, 3, 2, 1),
(4, 4, 2, 1),
(4, 5, 2, 1),
(4, 6, 2, 1),
(5, 1, 2, 1),
(5, 2, 2, 1),
(5, 3, 2, 1),
(5, 4, 2, 1),
(5, 5, 2, 1),
(5, 6, 2, 1),
(6, 1, 2, 1),
(6, 2, 2, 1),
(6, 3, 2, 1),
(6, 4, 2, 1),
(6, 5, 2, 1),
(6, 6, 2, 1),
(7, 1, 2, 1),
(7, 2, 2, 1),
(7, 3, 2, 1),
(7, 4, 2, 1),
(7, 5, 2, 1),
(7, 6, 2, 1);

-- --------------------------------------------------------

--
-- Table structure for table `pmt_alarms`
--

CREATE TABLE IF NOT EXISTS `pmt_alarms` (
  `program_number` int(11) NOT NULL,
  `input` int(10) NOT NULL,
  `alarm_enable` int(11) NOT NULL,
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`rmx_id`,`input`,`program_number`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `psi_si_interval`
--

CREATE TABLE IF NOT EXISTS `psi_si_interval` (
  `output` int(1) NOT NULL,
  `pat_int` int(6) NOT NULL,
  `sdt_int` int(6) NOT NULL,
  `nit_int` int(6) NOT NULL,
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`output`,`rmx_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `rf_authorization`
--

CREATE TABLE IF NOT EXISTS `rf_authorization` (
  `rmx_no` int(6) NOT NULL,
  `enable` int(1) NOT NULL,
  PRIMARY KEY (`rmx_no`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `rf_authorization`
--

INSERT INTO `rf_authorization` (`rmx_no`, `enable`) VALUES
(1, 1),
(2, 1),
(3, 1),
(4, 1),
(5, 1),
(6, 1);

-- --------------------------------------------------------

--
-- Table structure for table `rmx_mux_out`
--

CREATE TABLE IF NOT EXISTS `rmx_mux_out` (
  `bit` int(11) NOT NULL,
  `value` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `rmx_mux_out`
--

INSERT INTO `rmx_mux_out` (`bit`, `value`) VALUES
(0, 0),
(1, 0),
(2, 0),
(3, 0),
(4, 0),
(5, 0),
(6, 0),
(7, 0),
(8, 0),
(9, 0),
(10, 0),
(11, 0),
(12, 0),
(13, 0),
(14, 0),
(15, 0),
(16, 0),
(17, 0),
(18, 0),
(19, 0),
(20, 0),
(21, 0);

-- --------------------------------------------------------

--
-- Table structure for table `rmx_registers`
--

CREATE TABLE IF NOT EXISTS `rmx_registers` (
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  `cs` int(2) NOT NULL,
  `address` int(12) NOT NULL,
  `data` int(11) NOT NULL,
  PRIMARY KEY (`rmx_id`,`cs`,`address`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `rmx_registers`
--

INSERT INTO `rmx_registers` (`rmx_id`, `cs`, `address`, `data`) VALUES
(1, 11, 12, 4),
(1, 12, 12, 4),
(1, 13, 12, 4),
(1, 14, 12, 4),
(1, 15, 12, 4),
(1, 16, 12, 4),
(1, 17, 12, 4),
(1, 18, 12, 4),
(3, 11, 12, 4),
(3, 12, 12, 4),
(3, 13, 12, 4),
(3, 14, 12, 4),
(3, 15, 12, 4),
(3, 16, 12, 4),
(3, 17, 12, 4),
(3, 18, 12, 4),
(4, 11, 12, 4),
(4, 12, 12, 4),
(4, 13, 12, 4),
(4, 14, 12, 4),
(4, 15, 12, 4),
(4, 16, 12, 4),
(4, 17, 12, 4),
(4, 18, 12, 4),
(5, 11, 12, 4),
(5, 12, 12, 4),
(5, 13, 12, 4),
(5, 14, 12, 4),
(5, 15, 12, 4),
(5, 16, 12, 4),
(5, 17, 12, 4),
(5, 18, 12, 4),
(6, 11, 12, 4),
(6, 12, 12, 4),
(6, 13, 12, 4),
(6, 14, 12, 4),
(6, 15, 12, 4),
(6, 16, 12, 4),
(6, 17, 12, 4),
(6, 18, 12, 4);

-- --------------------------------------------------------

--
-- Table structure for table `service_providers`
--

CREATE TABLE IF NOT EXISTS `service_providers` (
  `service_number` int(11) NOT NULL,
  `provider_name` varchar(256) NOT NULL,
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`rmx_id`,`service_number`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `spts_ip_inputs`
--

CREATE TABLE IF NOT EXISTS `spts_ip_inputs` (
  `mux_id` int(1) NOT NULL,
  `channel_no` int(2) NOT NULL,
  `ip_address` varchar(11) NOT NULL,
  `port` int(10) NOT NULL,
  `type` int(1) NOT NULL,
  `is_enable` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`channel_no`,`mux_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `stream_service_map`
--

CREATE TABLE IF NOT EXISTS `stream_service_map` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `service_id` int(11) NOT NULL,
  `stream_id` int(11) NOT NULL,
  `channel_id` int(11) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `table_versions`
--

CREATE TABLE IF NOT EXISTS `table_versions` (
  `output` int(1) NOT NULL,
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  `pat_ver` int(11) NOT NULL,
  `pat_isenable` int(1) NOT NULL,
  `sdt_ver` int(11) NOT NULL,
  `sdt_isenable` int(1) NOT NULL,
  `nit_ver` int(11) NOT NULL,
  `nit_isenable` int(1) NOT NULL,
  PRIMARY KEY (`output`,`rmx_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `table_versions`
--

INSERT INTO `table_versions` (`output`, `rmx_id`, `pat_ver`, `pat_isenable`, `sdt_ver`, `sdt_isenable`, `nit_ver`, `nit_isenable`) VALUES
(1, 1, 1, 0, 1, 0, 2, 1);

-- --------------------------------------------------------

--
-- Table structure for table `tuner_details`
--

CREATE TABLE IF NOT EXISTS `tuner_details` (
  `mxl_id` int(11) NOT NULL,
  `rmx_no` int(11) NOT NULL,
  `demod_id` int(11) NOT NULL,
  `lnb_id` int(11) NOT NULL,
  `dvb_standard` int(11) NOT NULL,
  `frequency` int(11) NOT NULL,
  `symbol_rate` int(11) NOT NULL,
  `modulation` int(11) NOT NULL DEFAULT '0',
  `fec` int(11) NOT NULL DEFAULT '0',
  `rolloff` int(11) NOT NULL DEFAULT '0',
  `pilots` int(11) NOT NULL DEFAULT '2',
  `spectrum` int(11) NOT NULL DEFAULT '0',
  `lo_frequency` int(11) NOT NULL,
  `is_enable` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`mxl_id`,`rmx_no`,`demod_id`),
  KEY `mxl_id` (`mxl_id`),
  KEY `rmx_no` (`rmx_no`),
  KEY `demode_id` (`demod_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
