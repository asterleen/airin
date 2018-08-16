-- ----------------------------------------------------------
-- 
--     This is Airin 4, an advanced WebSocket chat server
--  Licensed under the new BSD 3-Clause license, see LICENSE
--        Made by Asterleen ~ https://asterleen.com
-- 
-- ----------------------------------------------------------
-- This is the PostgreSQL compatible database dump for Airin
-- Just import this file into your database. Don't forget to
-- set up privileges for the artifacts from this file! :3
-- ----------------------------------------------------------


-- This is a UNIX_TIMESTAMP polyfill from MySQL for PostgreSQL
-- It's easier to declare it here instead of doing that in the
-- Airin Server code
CREATE OR REPLACE FUNCTION unix_timestamp(ts timestamp without time zone)
  RETURNS integer AS
$BODY$
begin
    return extract(epoch from ts at time zone 'msk'); -- set your timezone here
end
$BODY$
  LANGUAGE plpgsql IMMUTABLE
  COST 100;


CREATE SEQUENCE seq_admin_users_id;
CREATE TABLE admin_users (
    id integer DEFAULT nextval('seq_admin_users_id'::regclass) NOT NULL,
    user_login character varying NOT NULL
);


CREATE TABLE auth (
    internal_token character varying NOT NULL,
    user_id character varying DEFAULT '0'::character varying NOT NULL,
    active boolean DEFAULT true NOT NULL,
    misc_info character varying
);


CREATE TABLE ban_states (
    ban_state_id integer NOT NULL,
    ban_state_tag character varying,
    ban_state_description character varying
);

-- These constants are defined in Airin server.
-- See 'airindata.h' file, enum AirinBanState
INSERT INTO ban_states (ban_state_id, ban_state_tag, ban_state_description)
VALUES
    (0, 'none', 'Not banned'),
    (1, 'shadow', 'Shadowbanned (silent mode)'),
    (2, 'full', 'Full ban, access is restricted');


CREATE SEQUENCE seq_bans_ban_id;
CREATE TABLE bans (
    ban_id integer DEFAULT nextval('seq_bans_ban_id'::regclass) NOT NULL,
    ban_login character varying NOT NULL,
    ban_comment character varying NOT NULL,
    ban_state integer
);


CREATE SEQUENCE seq_messages_message_id;
CREATE TABLE messages (
    message_id integer DEFAULT nextval('seq_messages_message_id'::regclass) NOT NULL,
    message_author_login character varying NOT NULL,
    message_author_name character varying,
    message_text character varying NOT NULL,
    message_visible boolean DEFAULT true NOT NULL,
    message_timestamp timestamp without time zone DEFAULT now() NOT NULL,
    message_name_color character varying
);


CREATE TABLE server_config (
    conf_name character varying NOT NULL,
    conf_value character varying
);


ALTER TABLE ONLY admin_users
    ADD CONSTRAINT admin_users_pkey PRIMARY KEY (id);
ALTER TABLE ONLY admin_users
    ADD CONSTRAINT admin_users_user_login_key UNIQUE (user_login);

ALTER TABLE ONLY auth
    ADD CONSTRAINT auth_pkey PRIMARY KEY (user_id);
ALTER TABLE ONLY auth
    ADD CONSTRAINT auth_user_id_key UNIQUE (user_id);

ALTER TABLE ONLY ban_states
    ADD CONSTRAINT ban_states_ban_state_tag_key UNIQUE (ban_state_tag);
ALTER TABLE ONLY ban_states
    ADD CONSTRAINT ban_states_pkey PRIMARY KEY (ban_state_id);

ALTER TABLE ONLY bans
    ADD CONSTRAINT bans_ban_login_key UNIQUE (ban_login);
ALTER TABLE ONLY bans
    ADD CONSTRAINT bans_pkey PRIMARY KEY (ban_id);

ALTER TABLE ONLY messages
    ADD CONSTRAINT messages_pkey PRIMARY KEY (message_id);

ALTER TABLE ONLY server_config
    ADD CONSTRAINT server_config_pkey PRIMARY KEY (conf_name);



ALTER TABLE ONLY bans
    ADD CONSTRAINT bans_ban_state_fkey FOREIGN KEY (ban_state) REFERENCES ban_states(ban_state_id) ON UPDATE CASCADE;

ALTER TABLE ONLY messages
    ADD CONSTRAINT messages_message_author_login_fkey FOREIGN KEY (message_author_login) REFERENCES auth(user_id) ON UPDATE CASCADE;