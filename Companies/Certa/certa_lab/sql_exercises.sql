-- =====================================================================
-- Certa FDE workbook -- SQL exercise suite
-- Verified against MySQL 9.7.1 (all syntax here is MySQL 8.0-compatible;
-- nothing outside 8.0 was used). Run:  mysql -u root --table < sql_exercises.sql
-- =====================================================================

DROP DATABASE IF EXISTS certa_prep;
CREATE DATABASE certa_prep CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci;
USE certa_prep;

SELECT @@version AS mysql_version, @@sql_mode AS sql_mode;

-- ---------------------------------------------------------------------
-- 1. Schema: mirrors the assessment's "count distinct vendors per region"
--    shape. The odd table name reproduces the quiz's generated name, and
--    the mixed-case columns reproduce the "Year / Region / VendorID" hints.
-- ---------------------------------------------------------------------
CREATE TABLE `maintable_NOG6Y` (
  `EntryID`    BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `Year`       SMALLINT        NOT NULL,
  `Region`     VARCHAR(40)     NULL,
  `VendorID`   VARCHAR(20)     NOT NULL,
  `VendorName` VARCHAR(120)    NOT NULL,
  `CreatedAt`  DATETIME        NOT NULL DEFAULT '2018-01-01 00:00:00',
  PRIMARY KEY (`EntryID`),
  KEY `idx_year_region` (`Year`, `Region`),
  KEY `idx_region_year` (`Region`, `Year`),
  KEY `idx_created_at` (`CreatedAt`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

INSERT INTO `maintable_NOG6Y` (`Year`, `Region`, `VendorID`, `VendorName`, `CreatedAt`) VALUES
  -- 2018, Europe: 3 unique vendors (V001,V002,V003) across 5 entries
  (2018, 'Europe',   'V001', 'Acme GmbH',              '2018-02-11 09:15:00'),
  (2018, 'Europe',   'V001', 'Acme GmbH',              '2018-06-04 14:02:00'),  -- same vendor, second entry
  (2018, 'Europe',   'V002', 'Northwind Analytics',    '2018-03-19 11:40:00'),
  (2018, 'Europe',   'V003', 'Baltic Logistics OU',    '2018-09-30 16:25:00'),
  (2018, 'Europe',   'V003', 'Baltic Logistics OU',    '2018-11-02 08:05:00'),  -- same vendor, second entry
  -- 2018, Americas: 3 unique vendors, 4 entries -> ties Europe on unique count,
  -- loses the tie-break because 4 < 5 entries
  (2018, 'Americas', 'V004', 'Globex Inc',             '2018-01-22 10:00:00'),
  (2018, 'Americas', 'V004', 'Globex Inc',             '2018-05-13 10:00:00'),
  (2018, 'Americas', 'V005', 'Initech LLC',            '2018-07-07 10:00:00'),
  (2018, 'Americas', 'V006', 'Umbrella Health',        '2018-08-18 10:00:00'),
  -- 2018, APAC: 2 unique vendors, 2 entries
  (2018, 'APAC',     'V007', 'Sakura Trading KK',      '2018-04-01 10:00:00'),
  (2018, 'APAC',     'V008', 'Merlion Systems Pte',    '2018-12-24 10:00:00'),
  -- 2018 row with an unknown region: NULL is a group of its own in GROUP BY
  (2018, NULL,       'V009', 'Unassigned Vendor Co',   '2018-10-10 10:00:00'),
  -- 2017 rows must be excluded by the year filter
  (2017, 'Europe',   'V010', 'Legacy Vendor Ltd',      '2017-02-14 10:00:00'),
  (2017, 'Europe',   'V010', 'Legacy Vendor Ltd',      '2017-05-14 10:00:00'),
  (2017, 'Europe',   'V010', 'Legacy Vendor Ltd',      '2017-09-14 10:00:00');

-- ---------------------------------------------------------------------
-- 2. THE ASSESSMENT QUERY (assessment topic 1)
--    "Count distinct vendor IDs and total entries per region for 2018,
--     sorted by unique vendors DESC, then total entries DESC."
-- ---------------------------------------------------------------------
SELECT '--- Ex 1: regional vendor counts for 2018 (model answer) ---' AS section;

SELECT
    `Region`,
    COUNT(DISTINCT `VendorID`) AS UniqueVendors,
    COUNT(*)                   AS TotalEntries
FROM `maintable_NOG6Y`
WHERE `Year` = 2018
GROUP BY `Region`
ORDER BY UniqueVendors DESC, TotalEntries DESC;

-- Semantics that change the result -- know these cold:
--   COUNT(*)               -> every row in the group, duplicates included
--   COUNT(`VendorID`)      -> non-NULL VendorID values (same as * here, NOT deduped)
--   COUNT(DISTINCT ...)    -> distinct non-NULL values; NULLs never counted
--   WHERE                  -> filters rows BEFORE grouping
--   HAVING                 -> filters groups AFTER aggregation
--   GROUP BY `Region`      -> NULL region becomes its own group
--   ORDER BY alias         -> allowed in MySQL; the alias resolves post-aggregation

SELECT '--- Ex 1a: COUNT(*) vs COUNT(col) vs COUNT(DISTINCT col) with NULLs ---' AS section;
SELECT
    COUNT(*)                        AS rows_total,
    COUNT(`Region`)                 AS rows_with_region,   -- NULL region excluded
    COUNT(DISTINCT `Region`)        AS distinct_regions,    -- NULL excluded again
    COUNT(`VendorID`)               AS non_null_vendor_ids,
    COUNT(DISTINCT `VendorID`)      AS distinct_vendors
FROM `maintable_NOG6Y`;

SELECT '--- Ex 1b: NULL region made explicit (reporting-friendly) ---' AS section;
SELECT
    COALESCE(`Region`, '(unassigned)') AS region_label,
    COUNT(DISTINCT `VendorID`)         AS UniqueVendors,
    COUNT(*)                           AS TotalEntries
FROM `maintable_NOG6Y`
WHERE `Year` = 2018
GROUP BY region_label
ORDER BY UniqueVendors DESC, TotalEntries DESC;

-- ---------------------------------------------------------------------
-- 3. Dimension join: report regions with zero entries (silent-zero trap)
-- ---------------------------------------------------------------------
SELECT '--- Ex 2: LEFT JOIN a region dimension so zero-count regions appear ---' AS section;

CREATE TABLE `regions` (
  `RegionCode` VARCHAR(40) NOT NULL,
  PRIMARY KEY (`RegionCode`)
) ENGINE=InnoDB;

INSERT INTO `regions` VALUES ('Europe'), ('Americas'), ('APAC'), ('MEA');

SELECT
    r.`RegionCode`                    AS Region,
    COUNT(DISTINCT v.`VendorID`)      AS UniqueVendors,
    COUNT(v.`EntryID`)                AS TotalEntries  -- COUNT(v.col), never COUNT(*)
FROM `regions` AS r
LEFT JOIN `maintable_NOG6Y` AS v
       ON v.`Region` = r.`RegionCode`
      AND v.`Year` = 2018
GROUP BY r.`RegionCode`
ORDER BY UniqueVendors DESC, TotalEntries DESC, Region ASC;

-- The classic bug: COUNT(*) after a LEFT JOIN returns 1 for the unmatched
-- MEA row, because the join emits one row with NULLs. COUNT(v.`EntryID`) is 0.
SELECT '--- Ex 2a: COUNT(*) vs COUNT(v.col) after a LEFT JOIN (the bug) ---' AS section;
SELECT
    r.`RegionCode`               AS Region,
    COUNT(*)                     AS wrong_count_star,
    COUNT(v.`EntryID`)           AS right_count_col
FROM `regions` AS r
LEFT JOIN `maintable_NOG6Y` AS v
       ON v.`Region` = r.`RegionCode` AND v.`Year` = 2018
GROUP BY r.`RegionCode`
ORDER BY Region;

-- ---------------------------------------------------------------------
-- 4. HAVING, shares, and the sargability trap
-- ---------------------------------------------------------------------
SELECT '--- Ex 3: HAVING filters groups (regions with >= 3 entries) ---' AS section;
SELECT `Region`, COUNT(*) AS TotalEntries
FROM `maintable_NOG6Y`
WHERE `Year` = 2018
GROUP BY `Region`
HAVING COUNT(*) >= 3
ORDER BY TotalEntries DESC;

SELECT '--- Ex 4: each region''s share of 2018 entries (window function) ---' AS section;
SELECT
    `Region`,
    COUNT(*) AS TotalEntries,
    ROUND(100 * COUNT(*) / SUM(COUNT(*)) OVER (), 1) AS pct_of_2018
FROM `maintable_NOG6Y`
WHERE `Year` = 2018
GROUP BY `Region`
ORDER BY TotalEntries DESC, `Region`;

SELECT '--- Ex 5: sargability -- YEAR(col) in WHERE disables the index ---' AS section;
-- Anti-pattern: wrapping the indexed column in a function makes the predicate
-- non-sargable, so MySQL cannot use idx_created_at and scans.
SELECT 'Non-sargable (index-unfriendly) form:' AS note;
EXPLAIN FORMAT=TRADITIONAL
SELECT COUNT(*) AS rows_2018_non_sargable FROM `maintable_NOG6Y` WHERE YEAR(`CreatedAt`) = 2018;

SELECT 'Sargable form (index-usable):' AS note;
EXPLAIN FORMAT=TRADITIONAL
SELECT COUNT(*) AS rows_2018_sargable
FROM `maintable_NOG6Y`
WHERE `CreatedAt` >= '2018-01-01 00:00:00' AND `CreatedAt` < '2019-01-01 00:00:00';

-- Bonus trap: YEAR(2018) is NULL, not 2018 -- YEAR() needs a DATE/DATETIME.
SELECT YEAR(2018) AS year_of_a_number_is_null, `Year` AS the_stored_year_column
FROM `maintable_NOG6Y` LIMIT 1;

-- ---------------------------------------------------------------------
-- 5. Window functions: running totals, top-N per group, dedupe
-- ---------------------------------------------------------------------
SELECT '--- Ex 6: cumulative vendor count across regions (default frame) ---' AS section;
SELECT
    `Region`,
    COUNT(DISTINCT `VendorID`) AS UniqueVendors,
    SUM(COUNT(DISTINCT `VendorID`)) OVER (ORDER BY `Region`) AS running_unique_vendors
FROM `maintable_NOG6Y`
WHERE `Year` = 2018
GROUP BY `Region`
ORDER BY `Region`;

SELECT '--- Ex 7: busiest region per year (ROW_NUMBER in a CTE) ---' AS section;
WITH per_year_region AS (
    SELECT `Year`, `Region`, COUNT(*) AS entries
    FROM `maintable_NOG6Y`
    WHERE `Region` IS NOT NULL
    GROUP BY `Year`, `Region`
),
ranked AS (
    SELECT `Year`, `Region`, entries,
           ROW_NUMBER() OVER (PARTITION BY `Year` ORDER BY entries DESC, `Region`) AS rn
    FROM per_year_region
)
SELECT `Year`, `Region`, entries
FROM ranked
WHERE rn = 1
ORDER BY `Year` DESC;

SELECT '--- Ex 8: GROUP BY ... WITH ROLLUP + GROUPING() to label the total ---' AS section;
SELECT
    CASE WHEN GROUPING(`Region`) = 1 THEN 'ALL REGIONS' ELSE COALESCE(`Region`, '(unassigned)') END AS Region,
    COUNT(DISTINCT `VendorID`) AS UniqueVendors,
    COUNT(*)                   AS TotalEntries
FROM `maintable_NOG6Y`
WHERE `Year` = 2018
GROUP BY `Region` WITH ROLLUP;

-- ---------------------------------------------------------------------
-- 6. Sync-shaped tables: upserts, dedupe, reconciliation
-- ---------------------------------------------------------------------
SELECT '--- Ex 9: idempotent upsert for a vendor sync (replay-safe) ---' AS section;

CREATE TABLE `vendor_sync` (
  `VendorID`   VARCHAR(20)  NOT NULL,
  `Region`     VARCHAR(40)  NOT NULL,
  `VendorName` VARCHAR(120) NOT NULL,
  `SourceVer`  INT UNSIGNED NOT NULL,          -- version from the source system
  `UpdatedAt`  TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `PayloadHash` CHAR(64)     NOT NULL,          -- sha256 of the mapped payload
  PRIMARY KEY (`VendorID`, `Region`)
) ENGINE=InnoDB;

INSERT INTO `vendor_sync` (`VendorID`, `Region`, `VendorName`, `SourceVer`, `PayloadHash`) VALUES
  ('V001', 'Europe', 'Acme GmbH',            5, SHA2('Acme GmbH|5', 256)),
  ('V002', 'Europe', 'Northwind Analytics',  2, SHA2('Northwind Analytics|2', 256));

-- Replaying the same event must not create a duplicate row and must not
-- clobber newer data. Version guard in the ON DUPLICATE clause.
INSERT INTO `vendor_sync` (`VendorID`, `Region`, `VendorName`, `SourceVer`, `PayloadHash`)
VALUES ('V001', 'Europe', 'Acme GmbH', 5, SHA2('Acme GmbH|5', 256))
ON DUPLICATE KEY UPDATE
  `VendorName`  = IF(VALUES(`SourceVer`) > `SourceVer`, VALUES(`VendorName`), `VendorName`),
  `PayloadHash` = IF(VALUES(`SourceVer`) > `SourceVer`, VALUES(`PayloadHash`), `PayloadHash`),
  `SourceVer`   = GREATEST(`SourceVer`, VALUES(`SourceVer`));

SELECT * FROM `vendor_sync` ORDER BY `VendorID`;
-- ROW_COUNT() semantics for INSERT ... ON DUPLICATE KEY UPDATE, read in the SAME
-- statement batch: 1 = inserted, 2 = updated, 0 = matched with no change.
-- It is per-connection and is reset by the next statement (a SELECT returns -1),
-- so never read it through a connection pool without care.

-- A stale replay (version 4 arriving after version 5) must be a no-op.
INSERT INTO `vendor_sync` (`VendorID`, `Region`, `VendorName`, `SourceVer`, `PayloadHash`)
VALUES ('V001', 'Europe', 'Old Acme Name', 4, SHA2('stale', 256))
ON DUPLICATE KEY UPDATE
  `VendorName` = IF(VALUES(`SourceVer`) > `SourceVer`, VALUES(`VendorName`), `VendorName`),
  `SourceVer`  = GREATEST(`SourceVer`, VALUES(`SourceVer`));

SELECT '--- Ex 9a: version 5 survived the stale version-4 replay ---' AS section;
SELECT `VendorID`, `VendorName`, `SourceVer` FROM `vendor_sync` WHERE `VendorID` = 'V001';

SELECT '--- Ex 10: dedupe -- keep the newest row per natural key ---' AS section;

CREATE TABLE `vendor_raw_landing` (
  `Id`         BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  `VendorID`   VARCHAR(20) NOT NULL,
  `Region`     VARCHAR(40) NOT NULL,
  `VendorName` VARCHAR(120) NOT NULL,
  `SourceVer`  INT NOT NULL,
  `IngestedAt` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

INSERT INTO `vendor_raw_landing` (`VendorID`, `Region`, `VendorName`, `SourceVer`) VALUES
  ('V001', 'Europe', 'Acme GmbH', 4),
  ('V001', 'Europe', 'Acme GmbH', 6),
  ('V001', 'Europe', 'Acme GmbH', 5),   -- arrives out of order
  ('V002', 'Europe', 'Northwind Analytics', 2);

WITH ranked AS (
    SELECT `Id`, `VendorID`, `Region`, `VendorName`, `SourceVer`,
           ROW_NUMBER() OVER (
               PARTITION BY `VendorID`, `Region`
               ORDER BY `SourceVer` DESC, `IngestedAt` DESC, `Id` DESC
           ) AS rn
    FROM `vendor_raw_landing`
)
SELECT `VendorID`, `Region`, `VendorName`, `SourceVer`
FROM ranked
WHERE rn = 1
ORDER BY `VendorID`;

SELECT '--- Ex 11: delete every duplicate, keep the winner (MySQL-safe self-join) ---' AS section;
DELETE l
FROM `vendor_raw_landing` AS l
JOIN `vendor_raw_landing` AS keep
  ON keep.`VendorID` = l.`VendorID`
 AND keep.`Region`   = l.`Region`
 AND (keep.`SourceVer` > l.`SourceVer`
      OR (keep.`SourceVer` = l.`SourceVer` AND keep.`Id` > l.`Id`));
SELECT COUNT(*) AS rows_after_dedupe FROM `vendor_raw_landing`;

SELECT '--- Ex 12: reconciliation -- field-level diff between Certa and the GRC ---' AS section;

CREATE TABLE `certa_vendors` (
  `VendorID` VARCHAR(20) PRIMARY KEY,
  `Name`     VARCHAR(120) NOT NULL,
  `Tier`     ENUM('low','medium','high','critical') NOT NULL,
  `Owner`    VARCHAR(80) NULL,
  `Version`  INT UNSIGNED NOT NULL
) ENGINE=InnoDB;

CREATE TABLE `grc_vendors` (
  `VendorID` VARCHAR(20) PRIMARY KEY,
  `Name`     VARCHAR(120) NOT NULL,
  `Tier`     ENUM('low','medium','high','critical') NOT NULL,
  `Owner`    VARCHAR(80) NULL,
  `Version`  INT UNSIGNED NOT NULL
) ENGINE=InnoDB;

INSERT INTO `certa_vendors` VALUES
  ('V001','Acme GmbH','high','ritik.r',7),
  ('V002','Northwind Analytics','medium',NULL,3),
  ('V003','Baltic Logistics OU','low','ops@certa',2),
  ('V004','Globex Inc','critical','sec-team',4);      -- only in Certa

INSERT INTO `grc_vendors` VALUES
  ('V001','Acme GmbH','high','someone.else@corp',5),  -- Owner differs -> conflict
  ('V002','Northwind Analytics','high',NULL,3),       -- Tier differs -> conflict
  ('V003','Baltic Logistics OU','low','ops@certa',2), -- in sync
  ('V009','Umbrella Health','medium','grc-owner',1);  -- only in GRC -> missing in Certa

-- 12a: presence diff in both directions, one pass
SELECT `VendorID`,
       CASE
         WHEN c.`VendorID` IS NULL THEN 'only_in_grc'
         WHEN g.`VendorID` IS NULL THEN 'only_in_certa'
         ELSE 'both'
       END AS presence
FROM `certa_vendors` AS c
LEFT JOIN `grc_vendors` AS g USING (`VendorID`)
UNION ALL
SELECT g.`VendorID`, 'only_in_grc'
FROM `grc_vendors` AS g
LEFT JOIN `certa_vendors` AS c USING (`VendorID`)
WHERE c.`VendorID` IS NULL
ORDER BY presence, `VendorID`;

-- 12b: field-level drift for rows present on both sides
SELECT c.`VendorID`,
       CASE WHEN c.`Name`  <> g.`Name`  THEN 'Name'  END AS name_drift,
       CASE WHEN c.`Tier`  <> g.`Tier`  THEN 'Tier'  END AS tier_drift,
       CASE WHEN COALESCE(c.`Owner`, '') <> COALESCE(g.`Owner`, '') THEN 'Owner' END AS owner_drift,
       c.`Version` AS certa_version, g.`Version` AS grc_version
FROM `certa_vendors` AS c
JOIN `grc_vendors`  AS g USING (`VendorID`)
WHERE c.`Name` <> g.`Name`
   OR c.`Tier` <> g.`Tier`
   OR COALESCE(c.`Owner`, '') <> COALESCE(g.`Owner`, '')
ORDER BY c.`VendorID`;

-- ---------------------------------------------------------------------
-- 7. Pagination: keyset vs OFFSET
-- ---------------------------------------------------------------------
SELECT '--- Ex 13: keyset (cursor) pagination -- stable under concurrent inserts ---' AS section;
-- Page 1
SELECT `EntryID`, `VendorID`, `Region` FROM `maintable_NOG6Y`
WHERE `Year` = 2018 AND `EntryID` > 0 ORDER BY `EntryID` LIMIT 5;
-- Page 2 (cursor = last EntryID seen)
SELECT `EntryID`, `VendorID`, `Region` FROM `maintable_NOG6Y`
WHERE `Year` = 2018 AND `EntryID` > 5 ORDER BY `EntryID` LIMIT 5;

SELECT '--- Ex 13a: OFFSET pagination skips/duplicates when rows are inserted ---' AS section;
SELECT `EntryID`, `VendorID` FROM `maintable_NOG6Y` WHERE `Year` = 2018 ORDER BY `EntryID` LIMIT 5 OFFSET 5;

-- ---------------------------------------------------------------------
-- 8. Concurrency: lost update vs conditional update vs row lock
-- ---------------------------------------------------------------------
SELECT '--- Ex 14: atomic increment (no read-modify-write race) ---' AS section;
CREATE TABLE `counters` (`Id` VARCHAR(20) PRIMARY KEY, `Value` INT NOT NULL);
INSERT INTO `counters` VALUES ('sync_attempts', 10);

-- BROKEN (two workers):  SELECT Value;  then  UPDATE counters SET Value = <read+1>
-- CORRECT: let the database do the arithmetic atomically
UPDATE `counters` SET `Value` = `Value` + 1 WHERE `Id` = 'sync_attempts';
SELECT * FROM `counters`;

SELECT '--- Ex 14a: optimistic concurrency -- conditional UPDATE, 0 rows means you lost ---' AS section;
CREATE TABLE `vendor_record` (
  `VendorID` VARCHAR(20) PRIMARY KEY,
  `Name`     VARCHAR(120) NOT NULL,
  `Version`  INT UNSIGNED NOT NULL
) ENGINE=InnoDB;
INSERT INTO `vendor_record` VALUES ('V001','Acme GmbH',1);

-- Worker A reads version 1, Worker B reads version 1. A writes first.
UPDATE `vendor_record` SET `Name` = 'Acme GmbH (A)', `Version` = `Version` + 1
WHERE `VendorID` = 'V001' AND `Version` = 1;
SELECT ROW_COUNT() AS worker_a_rows_updated;   -- 1 -> success

UPDATE `vendor_record` SET `Name` = 'Acme GmbH (B)', `Version` = `Version` + 1
WHERE `VendorID` = 'V001' AND `Version` = 1;
SELECT ROW_COUNT() AS worker_b_rows_updated;   -- 0 -> retry with fresh data

SELECT * FROM `vendor_record`;

-- The same guarantee is stronger with a row lock held across the read + write
-- in one transaction:
--   START TRANSACTION;
--   SELECT `Version` FROM `vendor_record` WHERE `VendorID` = 'V001' FOR UPDATE;  -- blocks competitors
--   UPDATE `vendor_record` SET `Version` = `Version` + 1 WHERE `VendorID` = 'V001';
--   COMMIT;
-- Deadlock risk exists only if two transactions lock rows in different orders:
-- always take locks in a deterministic order (e.g. ORDER BY VendorID).

SELECT '--- Ex 15: unique constraint is the only real duplicate guard ---' AS section;
CREATE TABLE `processed_events` (
  `ProviderEventID` VARCHAR(64) NOT NULL,
  `ReceivedAt`      TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`ProviderEventID`)           -- the dedupe guarantee lives here
) ENGINE=InnoDB;

INSERT IGNORE INTO `processed_events` (`ProviderEventID`) VALUES ('evt_1001');
INSERT IGNORE INTO `processed_events` (`ProviderEventID`) VALUES ('evt_1001');
SELECT ROW_COUNT() AS second_insert_ignored_returns_zero, (SELECT COUNT(*) FROM `processed_events`) AS rows_stored;

-- ---------------------------------------------------------------------
-- 9. JSON, generated columns, and defensive date handling
-- ---------------------------------------------------------------------
SELECT '--- Ex 16: JSON extraction into typed, indexable columns ---' AS section;
CREATE TABLE `webhook_events` (
  `EventID`   VARCHAR(64) PRIMARY KEY,
  `Payload`   JSON NOT NULL,
  `EventType` VARCHAR(40) GENERATED ALWAYS AS (JSON_UNQUOTE(JSON_EXTRACT(`Payload`, '$.type'))) STORED,
  `VendorID`  VARCHAR(40) GENERATED ALWAYS AS (JSON_UNQUOTE(JSON_EXTRACT(`Payload`, '$.data.vendor_id'))) STORED,
  KEY `idx_event_type` (`EventType`),
  KEY `idx_vendor` (`VendorID`)
) ENGINE=InnoDB;

INSERT INTO `webhook_events` (`EventID`, `Payload`) VALUES
  ('evt_1001', JSON_OBJECT('type','vendor.updated','data', JSON_OBJECT('vendor_id','V001','version',5))),
  ('evt_1002', JSON_OBJECT('type','vendor.created','data', JSON_OBJECT('vendor_id','V002','version',1))),
  ('evt_1003', JSON_OBJECT('type','vendor.updated','data', JSON_OBJECT('vendor_id','V001','version',3)));

SELECT `EventID`, `EventType`, `VendorID`,
       JSON_EXTRACT(`Payload`, '$.data.version') AS raw_json_version,          -- stays JSON
       `Payload` ->> '$.data.version'           AS text_version               -- ->> unquotes
FROM `webhook_events`
ORDER BY `VendorID`, (CAST(`Payload` ->> '$.data.version' AS UNSIGNED)) DESC;

SELECT '--- Ex 17: aggregate JSON by vendor (latest event wins) ---' AS section;
SELECT `VendorID`,
       MAX(CAST(`Payload` ->> '$.data.version' AS UNSIGNED)) AS highest_version,
       COUNT(*) AS events_seen
FROM `webhook_events`
GROUP BY `VendorID`
ORDER BY `VendorID`;

SELECT '--- Ex 18: defensive date parsing -- never trust the format ---' AS section;
SELECT
    STR_TO_DATE('2024-03-01', '%Y-%m-%d')               AS iso_date,
    STR_TO_DATE('28 February 2027', '%d %M %Y')         AS with_month_name,
    STR_TO_DATE('2027-02-30', '%Y-%m-%d')               AS impossible_date_becomes_null,
    (SELECT DATE('2024-02-29'))                          AS leap_day_ok,
    (SELECT DATE_ADD('2024-01-31', INTERVAL 1 MONTH))    AS month_math_clamps;

-- ---------------------------------------------------------------------
-- 10. Index + plan demonstration
-- ---------------------------------------------------------------------
SELECT '--- Ex 19: EXPLAIN with and without the composite index ---' AS section;
EXPLAIN FORMAT=TREE
SELECT `Region`, COUNT(DISTINCT `VendorID`)
FROM `maintable_NOG6Y`
WHERE `Year` = 2018
GROUP BY `Region`;

ALTER TABLE `maintable_NOG6Y` DROP INDEX `idx_year_region`;
SELECT 'After dropping idx_year_region (plan degrades to a scan):' AS note;
EXPLAIN FORMAT=TRADITIONAL
SELECT `Region`, COUNT(DISTINCT `VendorID`)
FROM `maintable_NOG6Y`
WHERE `Year` = 2018
GROUP BY `Region`;

ALTER TABLE `maintable_NOG6Y` ADD INDEX `idx_year_region` (`Year`, `Region`);
SELECT 'Index restored. Composite order matters: (Year, Region) serves WHERE Year=? GROUP BY Region;' AS note;

-- Why (Year, Region) and not (Region, Year):
--   the leftmost prefix must match the equality filter. (Region, Year) would
--   still need a full index scan to apply Year = 2018 unless Region is filtered.
SELECT '--- Ex 20: composite prefix rule ---' AS section;
EXPLAIN FORMAT=TRADITIONAL SELECT COUNT(*) FROM `maintable_NOG6Y` WHERE `Year` = 2018;              -- uses prefix
EXPLAIN FORMAT=TRADITIONAL SELECT COUNT(*) FROM `maintable_NOG6Y` WHERE `Region` = 'Europe';        -- uses idx_region_year
EXPLAIN FORMAT=TRADITIONAL SELECT COUNT(*) FROM `maintable_NOG6Y` WHERE `VendorName` = 'Acme GmbH'; -- full scan

-- ---------------------------------------------------------------------
-- 11. ONLY_FULL_GROUP_BY: what MySQL 8 rejects that MySQL 5.7 allowed
-- ---------------------------------------------------------------------
SELECT '--- Ex 21: strict grouping (expected to FAIL on purpose) ---' AS section;
-- SELECT `Region`, `VendorID`, COUNT(*) FROM `maintable_NOG6Y`
-- WHERE `Year` = 2018 GROUP BY `Region`;
-- MySQL 8 default sql_mode includes ONLY_FULL_GROUP_BY, so the above raises:
--   ERROR 1055 (42000): Expression #2 of SELECT list is not in GROUP BY clause and
--   contains nonaggregated column ... which is not functionally dependent on columns
--   in GROUP BY clause; this is incompatible with sql_mode=only_full_group_by
SELECT `Region`, COUNT(*) AS TotalEntries,
       MIN(`VendorID`) AS first_vendor, MAX(`VendorID`) AS last_vendor   -- explicit aggregate = portable
FROM `maintable_NOG6Y`
WHERE `Year` = 2018
GROUP BY `Region`
ORDER BY TotalEntries DESC;

SELECT '--- done ---' AS section;
