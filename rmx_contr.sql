-- phpMyAdmin SQL Dump
-- version 4.0.10deb1
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: Feb 08, 2018 at 03:39 PM
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

--
-- Dumping data for table `active_service_list`
--

INSERT INTO `active_service_list` (`in_channel`, `out_channel`, `channel_num`, `rmx_id`) VALUES
(0, 0, 1, 2),
(0, 0, 2, 2),
(0, 0, 3, 2),
(0, 0, 4, 2),
(0, 0, 5, 2),
(0, 0, 6, 1),
(0, 0, 7, 1),
(0, 0, 8, 1),
(0, 0, 9, 1),
(0, 0, 10, 1),
(0, 0, 11, 1),
(0, 0, 12, 1),
(0, 0, 13, 1),
(0, 0, 14, 1),
(0, 0, 15, 1),
(0, 0, 16, 1),
(0, 0, 17, 1),
(0, 0, 18, 1),
(0, 0, 19, 1),
(0, 0, 20, 3),
(0, 0, 21, 3),
(0, 0, 4401, 1),
(0, 0, 4402, 1),
(0, 0, 4411, 1),
(0, 0, 4412, 1),
(0, 0, 4413, 1),
(0, 0, 4414, 1),
(0, 0, 4415, 1),
(0, 0, 4416, 1),
(0, 0, 4422, 1),
(0, 0, 4430, 1),
(0, 0, 44412, 1),
(0, 1, 1, 2),
(0, 1, 2, 2),
(0, 1, 3, 2),
(0, 1, 4, 2),
(0, 1, 5, 2),
(0, 1, 20, 1),
(0, 1, 21, 1),
(0, 2, 1, 1),
(0, 2, 2, 1),
(0, 2, 3, 1),
(0, 2, 4, 1),
(0, 2, 20, 2),
(0, 2, 21, 2),
(0, 3, 20, 1),
(0, 3, 21, 1),
(1, 0, 1, 5),
(1, 0, 9, 1),
(1, 0, 10, 1),
(1, 0, 11, 1),
(1, 0, 12, 1),
(1, 0, 13, 1),
(1, 0, 14, 1),
(1, 0, 15, 1),
(1, 0, 16, 1),
(1, 0, 17, 1),
(1, 0, 18, 1),
(1, 0, 19, 1),
(1, 0, 20, 1),
(1, 0, 21, 1),
(1, 0, 23, 1),
(1, 0, 24, 1),
(1, 0, 26, 1),
(1, 0, 27, 1),
(1, 0, 29, 1),
(1, 0, 31, 1),
(1, 0, 32, 1),
(1, 0, 34, 1),
(1, 0, 35, 1),
(1, 0, 39, 1),
(1, 0, 40, 1),
(1, 0, 41, 1),
(1, 0, 4401, 1),
(1, 0, 4402, 1),
(1, 0, 4411, 1),
(1, 0, 4412, 1),
(1, 0, 4413, 1),
(1, 0, 4414, 1),
(1, 0, 4415, 1),
(1, 0, 4416, 1),
(1, 1, 1, 1),
(1, 1, 2, 1),
(1, 1, 4, 1),
(1, 1, 5, 1),
(1, 1, 6, 1),
(1, 1, 7, 1),
(1, 1, 8, 1),
(1, 1, 9, 1),
(1, 1, 15, 1),
(1, 1, 16, 1),
(1, 1, 17, 1),
(1, 1, 19, 1),
(1, 1, 21, 1),
(1, 1, 23, 1),
(1, 1, 24, 1),
(1, 1, 26, 1),
(1, 1, 27, 1),
(1, 1, 29, 1),
(1, 1, 31, 1),
(1, 1, 32, 1),
(1, 1, 34, 1),
(1, 1, 35, 1),
(1, 1, 39, 1),
(1, 1, 40, 1),
(1, 1, 41, 1),
(3, 3, 1, 1),
(3, 3, 2, 1),
(3, 3, 3, 1),
(3, 3, 9, 1),
(3, 3, 10, 1),
(3, 3, 11, 1),
(4, 0, 1, 5),
(4, 0, 2, 5),
(4, 0, 3, 5),
(4, 0, 4, 5),
(4, 0, 5, 5),
(4, 0, 6, 5),
(4, 0, 7, 5),
(4, 0, 8, 5),
(4, 0, 9, 5),
(4, 0, 10, 5),
(4, 0, 11, 5),
(4, 0, 12, 5),
(4, 0, 13, 5),
(4, 0, 14, 5),
(4, 0, 15, 5),
(4, 0, 16, 6),
(4, 0, 17, 6),
(4, 0, 18, 6),
(4, 0, 19, 6),
(4, 0, 20, 6),
(4, 1, 6, 6),
(4, 1, 7, 6),
(4, 1, 8, 6),
(4, 1, 9, 6),
(4, 1, 10, 6);

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

--
-- Dumping data for table `channel_list`
--

INSERT INTO `channel_list` (`input_channel`, `channel_number`, `rmx_id`) VALUES
(0, 1, 1),
(0, 2, 1),
(0, 3, 1),
(0, 4, 1),
(0, 5, 1),
(0, 6, 1),
(0, 7, 1),
(0, 8, 1),
(0, 9, 1),
(0, 10, 1),
(0, 11, 1),
(0, 12, 1),
(0, 13, 1),
(0, 14, 1),
(0, 15, 1),
(0, 16, 1),
(0, 17, 1),
(0, 18, 1),
(0, 19, 1),
(0, 20, 1),
(0, 21, 1),
(0, 4401, 1),
(0, 4402, 1),
(0, 4411, 1),
(0, 4412, 1),
(0, 4413, 1),
(0, 4414, 1),
(0, 4415, 1),
(0, 4416, 1),
(0, 4422, 1),
(0, 4430, 1),
(0, 4435, 1),
(0, 4440, 1),
(1, 1, 1),
(1, 2, 1),
(1, 3, 1),
(1, 4, 1),
(1, 5, 1),
(1, 6, 1),
(1, 7, 1),
(1, 8, 1),
(1, 9, 1),
(1, 10, 1),
(1, 11, 1),
(1, 12, 1),
(1, 13, 1),
(1, 14, 1),
(1, 15, 1),
(1, 16, 1),
(1, 17, 1),
(1, 18, 1),
(1, 19, 1),
(1, 20, 1),
(1, 21, 1),
(1, 23, 1),
(1, 24, 1),
(1, 26, 1),
(1, 27, 1),
(1, 29, 1),
(1, 31, 1),
(1, 32, 1),
(1, 34, 1),
(1, 35, 1),
(1, 39, 1),
(1, 40, 1),
(1, 41, 1),
(1, 4401, 1),
(1, 4402, 1),
(1, 4411, 1),
(1, 4412, 1),
(1, 4413, 1),
(1, 4414, 1),
(1, 4415, 1),
(1, 4416, 1),
(1, 4422, 1),
(1, 4430, 1),
(1, 4435, 1),
(1, 4440, 1),
(2, 1, 1),
(2, 2, 1),
(2, 3, 1),
(2, 4, 1),
(2, 5, 1),
(2, 6, 1),
(2, 7, 1),
(2, 8, 1),
(2, 9, 1),
(2, 10, 1),
(2, 11, 1),
(2, 12, 1),
(2, 13, 1),
(2, 14, 1),
(2, 15, 1),
(2, 16, 1),
(2, 17, 1),
(2, 18, 1),
(2, 19, 1),
(2, 20, 1),
(2, 123, 1),
(2, 4401, 1),
(2, 4402, 1),
(2, 4411, 1),
(2, 4412, 1),
(2, 4413, 1),
(2, 4414, 1),
(2, 4415, 1),
(2, 4416, 1),
(2, 4422, 1),
(2, 4430, 1),
(2, 4435, 1),
(2, 4440, 1),
(2, 9991, 1),
(3, 1, 1),
(3, 2, 1),
(3, 3, 1),
(3, 4, 1),
(3, 5, 1),
(3, 6, 1),
(3, 7, 1),
(3, 8, 1),
(3, 9, 1),
(3, 10, 1),
(3, 11, 1),
(3, 12, 1),
(3, 13, 1),
(3, 14, 1),
(3, 15, 1),
(3, 16, 1),
(3, 17, 1),
(3, 18, 1),
(3, 19, 1),
(3, 20, 1),
(3, 21, 1),
(3, 4401, 1),
(3, 4402, 1),
(3, 4411, 1),
(3, 4412, 1),
(3, 4413, 1),
(3, 4414, 1),
(3, 4415, 1),
(3, 4416, 1),
(3, 4422, 1),
(3, 4430, 1),
(3, 4435, 1),
(3, 4440, 1),
(4, 4401, 1),
(4, 4402, 1),
(4, 4411, 1),
(4, 4412, 1),
(4, 4413, 1),
(4, 4414, 1),
(4, 4415, 1),
(4, 4416, 1),
(4, 4422, 1),
(4, 4430, 1),
(4, 4435, 1),
(4, 4440, 1),
(5, 4401, 1),
(5, 4402, 1),
(5, 4411, 1),
(5, 4412, 1),
(5, 4413, 1),
(5, 4414, 1),
(5, 4415, 1),
(5, 4416, 1),
(5, 4422, 1),
(5, 4430, 1),
(5, 4435, 1),
(5, 4440, 1),
(6, 4401, 1),
(6, 4402, 1),
(6, 4411, 1),
(6, 4412, 1),
(6, 4413, 1),
(6, 4414, 1),
(6, 4415, 1),
(6, 4416, 1),
(6, 4422, 1),
(6, 4430, 1),
(6, 4435, 1),
(6, 4440, 1),
(7, 4401, 1),
(7, 4402, 1),
(7, 4411, 1),
(7, 4412, 1),
(7, 4413, 1),
(7, 4414, 1),
(7, 4415, 1),
(7, 4416, 1),
(7, 4422, 1),
(7, 4430, 1),
(7, 4435, 1),
(7, 4440, 1),
(0, 1, 2),
(0, 2, 2),
(0, 3, 2),
(0, 4, 2),
(0, 5, 2),
(0, 6, 2),
(0, 7, 2),
(0, 8, 2),
(0, 9, 2),
(0, 10, 2),
(0, 11, 2),
(0, 12, 2),
(0, 13, 2),
(0, 14, 2),
(0, 15, 2),
(0, 16, 2),
(0, 17, 2),
(0, 18, 2),
(0, 19, 2),
(0, 20, 2),
(0, 21, 2),
(0, 4401, 2),
(0, 4402, 2),
(0, 4411, 2),
(0, 4412, 2),
(0, 4413, 2),
(0, 4414, 2),
(0, 4415, 2),
(0, 4416, 2),
(0, 4422, 2),
(0, 4430, 2),
(0, 4435, 2),
(0, 4440, 2),
(2, 4401, 2),
(2, 4402, 2),
(2, 4411, 2),
(2, 4412, 2),
(2, 4413, 2),
(2, 4414, 2),
(2, 4415, 2),
(2, 4416, 2),
(2, 4422, 2),
(2, 4430, 2),
(2, 4435, 2),
(2, 4440, 2),
(4, 4401, 2),
(4, 4402, 2),
(4, 4411, 2),
(4, 4412, 2),
(4, 4413, 2),
(4, 4414, 2),
(4, 4415, 2),
(4, 4416, 2),
(4, 4422, 2),
(4, 4430, 2),
(4, 4435, 2),
(4, 4440, 2),
(5, 4401, 2),
(5, 4402, 2),
(5, 4411, 2),
(5, 4412, 2),
(5, 4413, 2),
(5, 4414, 2),
(5, 4415, 2),
(5, 4416, 2),
(5, 4422, 2),
(5, 4430, 2),
(5, 4435, 2),
(5, 4440, 2),
(6, 4401, 2),
(6, 4402, 2),
(6, 4411, 2),
(6, 4412, 2),
(6, 4413, 2),
(6, 4414, 2),
(6, 4415, 2),
(6, 4416, 2),
(6, 4422, 2),
(6, 4430, 2),
(6, 4435, 2),
(6, 4440, 2),
(7, 4401, 2),
(7, 4402, 2),
(7, 4411, 2),
(7, 4412, 2),
(7, 4413, 2),
(7, 4414, 2),
(7, 4415, 2),
(7, 4416, 2),
(7, 4422, 2),
(7, 4430, 2),
(7, 4435, 2),
(7, 4440, 2),
(0, 1, 3),
(0, 2, 3),
(0, 3, 3),
(0, 4, 3),
(0, 5, 3),
(0, 6, 3),
(0, 7, 3),
(0, 8, 3),
(0, 9, 3),
(0, 10, 3),
(0, 11, 3),
(0, 12, 3),
(0, 13, 3),
(0, 14, 3),
(0, 15, 3),
(0, 16, 3),
(0, 17, 3),
(0, 18, 3),
(0, 19, 3),
(0, 20, 3),
(0, 21, 3),
(0, 4401, 5),
(0, 4402, 5),
(0, 4411, 5),
(0, 4412, 5),
(0, 4413, 5),
(0, 4414, 5),
(0, 4415, 5),
(0, 4416, 5),
(0, 4422, 5),
(0, 4430, 5),
(0, 4435, 5),
(0, 4440, 5),
(1, 1, 5),
(1, 4401, 5),
(1, 4402, 5),
(1, 4411, 5),
(1, 4412, 5),
(1, 4413, 5),
(1, 4414, 5),
(1, 4415, 5),
(1, 4416, 5),
(1, 4422, 5),
(1, 4430, 5),
(1, 4435, 5),
(1, 4440, 5),
(2, 1, 5),
(2, 4401, 5),
(2, 4402, 5),
(2, 4411, 5),
(2, 4412, 5),
(2, 4413, 5),
(2, 4414, 5),
(2, 4415, 5),
(2, 4416, 5),
(2, 4422, 5),
(2, 4430, 5),
(2, 4435, 5),
(2, 4440, 5),
(3, 1, 5),
(3, 4401, 5),
(3, 4402, 5),
(3, 4411, 5),
(3, 4412, 5),
(3, 4413, 5),
(3, 4414, 5),
(3, 4415, 5),
(3, 4416, 5),
(3, 4422, 5),
(3, 4430, 5),
(3, 4435, 5),
(3, 4440, 5),
(4, 1, 5),
(4, 2, 5),
(4, 3, 5),
(4, 4, 5),
(4, 5, 5),
(4, 6, 5),
(4, 7, 5),
(4, 8, 5),
(4, 9, 5),
(4, 10, 5),
(4, 11, 5),
(4, 12, 5),
(4, 13, 5),
(4, 14, 5),
(4, 15, 5),
(4, 16, 5),
(4, 17, 5),
(4, 18, 5),
(4, 19, 5),
(4, 20, 5),
(4, 21, 5),
(4, 4401, 5),
(4, 4402, 5),
(4, 4411, 5),
(4, 4412, 5),
(4, 4413, 5),
(4, 4414, 5),
(4, 4415, 5),
(4, 4416, 5),
(4, 4422, 5),
(4, 4430, 5),
(4, 4435, 5),
(4, 4440, 5),
(7, 4401, 5),
(7, 4402, 5),
(7, 4411, 5),
(7, 4412, 5),
(7, 4413, 5),
(7, 4414, 5),
(7, 4415, 5),
(7, 4416, 5),
(7, 4422, 5),
(7, 4430, 5),
(7, 4435, 5),
(7, 4440, 5),
(0, 4401, 6),
(0, 4402, 6),
(0, 4411, 6),
(0, 4412, 6),
(0, 4413, 6),
(0, 4414, 6),
(0, 4415, 6),
(0, 4416, 6),
(0, 4422, 6),
(0, 4430, 6),
(0, 4435, 6),
(0, 4440, 6),
(2, 4401, 6),
(2, 4402, 6),
(2, 4411, 6),
(2, 4412, 6),
(2, 4413, 6),
(2, 4414, 6),
(2, 4415, 6),
(2, 4416, 6),
(2, 4422, 6),
(2, 4430, 6),
(2, 4435, 6),
(2, 4440, 6),
(4, 1, 6),
(4, 2, 6),
(4, 3, 6),
(4, 4, 6),
(4, 5, 6),
(4, 6, 6),
(4, 7, 6),
(4, 8, 6),
(4, 9, 6),
(4, 10, 6),
(4, 11, 6),
(4, 12, 6),
(4, 13, 6),
(4, 14, 6),
(4, 15, 6),
(4, 16, 6),
(4, 17, 6),
(4, 18, 6),
(4, 19, 6),
(4, 20, 6),
(4, 21, 6),
(4, 4401, 6),
(4, 4402, 6),
(4, 4411, 6),
(4, 4412, 6),
(4, 4413, 6),
(4, 4414, 6),
(4, 4415, 6),
(4, 4416, 6),
(4, 4422, 6),
(4, 4430, 6),
(4, 4435, 6),
(4, 4440, 6);

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
  PRIMARY KEY (`channel_id`)
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

--
-- Dumping data for table `freeCA_mode_programs`
--

INSERT INTO `freeCA_mode_programs` (`program_number`, `input_channel`, `output_channel`, `rmx_id`) VALUES
(7, 0, 3, 1);

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
(1, 100),
(2, 300);

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
(1, 1, 1),
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
(4, 1, 1),
(4, 2, 0),
(4, 3, 1),
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

--
-- Dumping data for table `ip_input_channels`
--

INSERT INTO `ip_input_channels` (`rmx_no`, `input_channel`, `ip_address`, `port`, `type`, `is_enable`) VALUES
(1, 2, '4026466561', 10002, 1, 1),
(3, 5, '4026466561', 10003, 1, 1);

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

--
-- Dumping data for table `ip_output_channels`
--

INSERT INTO `ip_output_channels` (`rmx_no`, `output_channel`, `ip_address`, `port`, `is_enable`) VALUES
(1, 1, '4026466562', 10002, 1),
(2, 1, '4026466561', 10001, 1);

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
-- Table structure for table `nit_mode`
--

CREATE TABLE IF NOT EXISTS `nit_mode` (
  `output` int(1) NOT NULL,
  `mode` int(11) NOT NULL,
  `rmx_id` int(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`rmx_id`,`output`)
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
  PRIMARY KEY (`output_channel`,`rmx_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `output`
--

INSERT INTO `output` (`output_channel`, `rmx_id`) VALUES
(0, 1),
(0, 2),
(0, 3),
(0, 4),
(0, 5),
(0, 6),
(1, 1),
(1, 2),
(1, 3),
(1, 4),
(1, 5),
(1, 6),
(2, 1),
(2, 2),
(2, 3),
(2, 4),
(2, 5),
(2, 6),
(3, 1),
(3, 2),
(3, 3),
(3, 4),
(3, 5),
(3, 6);

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
(5, 1),
(6, 1);

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
(1, 11, 12, 8),
(1, 14, 12, 10);

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

--
-- Dumping data for table `tuner_details`
--

INSERT INTO `tuner_details` (`mxl_id`, `rmx_no`, `demod_id`, `lnb_id`, `dvb_standard`, `frequency`, `symbol_rate`, `modulation`, `fec`, `rolloff`, `pilots`, `spectrum`, `lo_frequency`, `is_enable`) VALUES
(1, 1, 0, 0, 2, 1274000000, 14300, 0, 0, 0, 2, 0, 5150, 1),
(1, 1, 3, 0, 2, 1274000000, 14300, 0, 0, 0, 2, 0, 5150, 1);

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
