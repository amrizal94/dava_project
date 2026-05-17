-- PostgreSQL database dump
-- Database: dava_monitoring
-- Dumped from database version 14.22 (Ubuntu 14.22-0ubuntu0.22.04.1)

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

-- Type: lightmode
CREATE TYPE public.lightmode AS ENUM (
    'auto',
    'manual'
);

-- Table: devices
CREATE TABLE public.devices (
    mac_address       character varying(17)        NOT NULL,
    alias             character varying(64)         NOT NULL,
    location          character varying(128)        NOT NULL,
    is_online         boolean                       NOT NULL,
    last_seen         timestamp with time zone,
    firmware_version  character varying(32)         NOT NULL,
    created_at        timestamp with time zone      NOT NULL,
    CONSTRAINT devices_pkey PRIMARY KEY (mac_address)
);

-- Table: device_settings
CREATE TABLE public.device_settings (
    device_mac        character varying(17)        NOT NULL,
    light_mode        public.lightmode             NOT NULL,
    light_brightness  integer                      NOT NULL,
    updated_at        timestamp with time zone     NOT NULL,
    CONSTRAINT device_settings_pkey PRIMARY KEY (device_mac),
    CONSTRAINT device_settings_device_mac_fkey
        FOREIGN KEY (device_mac) REFERENCES public.devices(mac_address) ON DELETE CASCADE
);

-- Table: sensor_readings
CREATE TABLE public.sensor_readings (
    id           integer                   NOT NULL DEFAULT nextval('public.sensor_readings_id_seq'::regclass),
    device_mac   character varying(17)    NOT NULL,
    "timestamp"  timestamp with time zone NOT NULL,
    temp_indoor  double precision,
    temp_outdoor double precision,
    lux          double precision,
    voltage      double precision,
    current      double precision,
    power        double precision,
    frequency    double precision,
    power_factor double precision,
    power_status boolean,
    CONSTRAINT sensor_readings_pkey PRIMARY KEY (id),
    CONSTRAINT sensor_readings_device_mac_fkey
        FOREIGN KEY (device_mac) REFERENCES public.devices(mac_address) ON DELETE CASCADE
);

CREATE SEQUENCE public.sensor_readings_id_seq
    AS integer START WITH 1 INCREMENT BY 1
    NO MINVALUE NO MAXVALUE CACHE 1;

ALTER SEQUENCE public.sensor_readings_id_seq OWNED BY public.sensor_readings.id;
