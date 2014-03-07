-- script to generate cities table for PostGis
-- used in examples 

-- Source: http://www.realestate3d.com/gps/latlong.htm

create user ggl password 'ggl' createdb;
create database ggl owner=ggl template=postgis;


drop table  if exists  cities;
create table cities(id serial primary key, name varchar(25));

select addgeometrycolumn('','cities','location','4326','POINT',2);
insert into cities(location, name) values(GeometryFromText('POINT( -71.03 42.37)', 4326), 'Boston');
insert into cities(location, name) values(GeometryFromText('POINT( -87.65 41.90)', 4326), 'Chicago');
insert into cities(location, name) values(GeometryFromText('POINT( -95.35 29.97)', 4326), 'Houston');
insert into cities(location, name) values(GeometryFromText('POINT(-118.40 33.93)', 4326), 'Los Angeles');
insert into cities(location, name) values(GeometryFromText('POINT( -80.28 25.82)', 4326), 'Miami');
insert into cities(location, name) values(GeometryFromText('POINT( -73.98 40.77)', 4326), 'New York');
insert into cities(location, name) values(GeometryFromText('POINT(-112.02 33.43)', 4326), 'Phoenix');
insert into cities(location, name) values(GeometryFromText('POINT( -77.04 38.85)', 4326), 'Washington DC');
