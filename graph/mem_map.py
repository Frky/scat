#!/usr/bin/python
# -*- coding: utf-8 -*-

def parse_data():
    vpath = dict()
    seen = list()
    with open("data_0.log", "r") as f:
        for line in f.readlines():
            v, f, fc, g, gc = line[:-1].split(",")
            v = int(v)
            if v not in vpath.keys():
                vpath[v] = 0
            vpath[v] += 1
    return vpath

def html_format(v, heat):
    return "<td>{0}:{1}</td>".format(v, heat)

def html_export(data):
    WIDTH = 3
    mini = min(data.keys())
    maxi = max(data.keys())
    print "<table>"
    for i, v in enumerate(xrange(mini, maxi + 1)):
        if i % WIDTH == 0:
            print "<tr>"
        if v in data.keys():
            print html_format(v, data[v])
        else:
            print html_format(v, 0)
        if i % WIDTH == WIDTH - 1:
            print "</tr>"
    print "</table>"

html_export(parse_data())
