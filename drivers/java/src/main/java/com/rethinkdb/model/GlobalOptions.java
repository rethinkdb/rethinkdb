// Autogenerated by metajava.py.
// Do not edit this file directly.
// The template for this file is located at:
// ../../../../../../../templates/GlobalOptions.java
package com.rethinkdb.model;

import com.rethinkdb.ast.helper.OptArgs;

import java.util.*;


public class GlobalOptions {

    private double _arrayLimit;
    private String _binaryFormat;
    private String _db;
    private String _durability;
    private double _firstBatchScaledownFactor;
    private String _groupFormat;
    private double _maxBatchBytes;
    private double _maxBatchRows;
    private double _maxBatchSeconds;
    private double _minBatchRows;
    private boolean _noreply;
    private boolean _profile;
    private String _timeFormat;
    private boolean _useOutdated;

    public GlobalOptions() {
        _arrayLimit = 100000;
        _binaryFormat = "native";
        _db = "test";
        _durability = "hard";
        _firstBatchScaledownFactor = 4;
        _groupFormat = "native";
        _maxBatchBytes = 1024;
        _maxBatchRows = null;
        _maxBatchSeconds = 0.5;
        _minBatchRows = 8;
        _noreply = false;
        _profile = false;
        _timeFormat = "native";
        _useOutdated = false;
    }

    public OptArgs toOptArgs() {
        return (new OptArgs())
            .with("array_limit", _arrayLimit)
            .with("binary_format", _binaryFormat)
            .with("db", _db)
            .with("durability", _durability)
            .with("first_batch_scaledown_factor", _firstBatchScaledownFactor)
            .with("group_format", _groupFormat)
            .with("max_batch_bytes", _maxBatchBytes)
            .with("max_batch_rows", _maxBatchRows)
            .with("max_batch_seconds", _maxBatchSeconds)
            .with("min_batch_rows", _minBatchRows)
            .with("noreply", _noreply)
            .with("profile", _profile)
            .with("time_format", _timeFormat)
            .with("use_outdated", _useOutdated)
        ;
    }

    public GlobalOptions arrayLimit(double arrayLimit) {
        _arrayLimit = arrayLimit;
        return this;
    }

    public GlobalOptions binaryFormat(String binaryFormat) {
        _binaryFormat = binaryFormat;
        return this;
    }

    public GlobalOptions db(String db) {
        _db = db;
        return this;
    }

    public GlobalOptions durability(String durability) {
        _durability = durability;
        return this;
    }

    public GlobalOptions firstBatchScaledownFactor(double firstBatchScaledownFactor) {
        _firstBatchScaledownFactor = firstBatchScaledownFactor;
        return this;
    }

    public GlobalOptions groupFormat(String groupFormat) {
        _groupFormat = groupFormat;
        return this;
    }

    public GlobalOptions maxBatchBytes(double maxBatchBytes) {
        _maxBatchBytes = maxBatchBytes;
        return this;
    }

    public GlobalOptions maxBatchRows(double maxBatchRows) {
        _maxBatchRows = maxBatchRows;
        return this;
    }

    public GlobalOptions maxBatchSeconds(double maxBatchSeconds) {
        _maxBatchSeconds = maxBatchSeconds;
        return this;
    }

    public GlobalOptions minBatchRows(double minBatchRows) {
        _minBatchRows = minBatchRows;
        return this;
    }

    public GlobalOptions noreply(boolean noreply) {
        _noreply = noreply;
        return this;
    }

    public GlobalOptions profile(boolean profile) {
        _profile = profile;
        return this;
    }

    public GlobalOptions timeFormat(String timeFormat) {
        _timeFormat = timeFormat;
        return this;
    }

    public GlobalOptions useOutdated(boolean useOutdated) {
        _useOutdated = useOutdated;
        return this;
    }


}
