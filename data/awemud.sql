CREATE TABLE objects (id INTEGER PRIMARY KEY AUTOINCREMENT, class VARCHAR(40) NOT NULL);

CREATE TABLE properties (id INTEGER NOT NULL, name VARCHAR(40) NOT NULL, value TEXT);
CREATE INDEX properties_id ON properties (id);

CREATE TABLE tags (id INTEGER NOT NULL, tag VARCHAR(40) NOT NULL, PRIMARY KEY(id,tag));
CREATE INDEX tags_tag ON tags (tag);
CREATE INDEX tags_id ON tags (id);

CREATE TABLE accounts (name VARCHAR(32) NOT NULL, password VARCHAR(64) NOT NULL, PRIMARY KEY(name));
INSERT INTO accounts (name, password) VALUES('admin', '$1$tNUNjzkD$F.tONxYNdR8hpYwuekqr7/');
