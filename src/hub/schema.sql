DROP TABLE IF EXISTS hub;
DROP TABLE IF EXISTS circuit;
DROP TABLE IF EXISTS member;
DROP TABLE IF EXISTS cabal;
DROP TABLE IF EXISTS service;
DROP TABLE IF EXISTS hate;
DROP TABLE IF EXISTS message;
DROP TABLE IF EXISTS message_type;


DROP TABLE IF EXISTS member_message;
DROP TABLE IF EXISTS cabal_member;
DROP TABLE IF EXISTS hub_service;


CREATE TABLE hub (name TEXT, key BLOB);

CREATE TABLE circuit (from_member_id INTEGER, to_member_id INTEGER);

CREATE TABLE member (hub_id INTEGER, name TEXT, key BLOB, last_active DATETIME, mean_hate REAL, last_conn_state INTEGER);

CREATE TABLE cabal (hub_id INTEGER, name TEXT);
CREATE TABLE cabal_member (cabal_id INTEGER, member_id INTEGER);

CREATE TABLE service (name TEXT, read_queue INTEGER);
CREATE TABLE hub_service (hub_id INTEGER, service_id INTEGER);

CREATE TABLE hate (member_id INTEGER, level INTEGER, nonce BLOB, hash BLOB, signature BLOB, is_challenge boolean);

CREATE TABLE message (circuit_id INTEGER, sequence_id INTEGER, target TEXT, type_id INTEGER, received_at DATETIME, data BLOB, from_member_id INTEGER);
CREATE TABLE member_message (member_id INTEGER, message_id INTEGER);
CREATE TABLE message_type (name TEXT);

