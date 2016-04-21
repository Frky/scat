#-*- coding: utf-8 -*-

from random import shuffle

import networkx as nx
import numpy as np
import matplotlib.pyplot as plt

COLORS = [
            '#1abc9c',
            '#2ecc71', 
            '#16a085',
            '#27ae60',
            '#3498db',
            '#2980b9',
            '#9b59b6',
            '#8e44ad',
            '#34495e',
            '#2c3e50',
            '#f1c40f',
            '#f39c12',
            '#e67e22',
            '#d35400',
            '#e74c3c',
            '#c0392b'
        ]

shuffle(COLORS)

def parse_data():
    vpath = dict()
    seen = list()
    with open("data_0.log", "r") as f:
        for line in f.readlines():
            v, f, fc, g, gc = line[:-1].split(",")
            if v not in vpath.keys():
                vpath[v] = list()
            vpath[v].append((f, fc))
            vpath[v].append((g, gc))
    with open("data.log", "w") as f:
        for v, path in vpath.items():
            vpath[v] = sorted(list(set(vpath[v])), key=lambda a:a[1])
            path = "*".join([a[0] for a in vpath[v]])
            if path not in seen:
                f.write(path + "\n")
                seen.append(path)

def parse_chain():
    SEPARATORS = ['*', '+']
    data = list()
    with open("data.log", "r") as f:
        for line in f.readlines():
            chain = list()
            fname = ""
            gname = ""
            op = ""
            for c in line[:-1]:
                if c not in SEPARATORS:
                    gname += c
                else: 
                    if fname != "":
                        chain.append((fname, op, gname))
                    fname = str(gname)
                    op = c
                    gname = ""
            chain.append((fname, op, gname))
            data.append(chain)
    return data

data = parse_data()
data = parse_chain()

G = nx.MultiDiGraph()
red_edges = list()

clrs = list()

for i, chain in enumerate(data):
    color = COLORS.pop()
    for couple in chain:
        style = "dotted" if couple[1] == "+" else "default"
        G.add_edges_from(
                             [(couple[0], couple[2])],
                             color=color,
                             style=style,
                )
        clrs.append(color)
pos = nx.spring_layout(G)
A = nx.nx_agraph.to_agraph(G)
for n, p in pos.items():
    A.get_node(n).attr['pos']='{},{}!'.format(p[0]*5, p[1]*5)
A.layout()
A.draw("graph.png")
exit()
pos=nx.spring_layout(G)
nx.draw_networkx_nodes(G, pos)
nx.draw_networkx_edges(G, pos, edge_color=clrs)
plt.show()
exit()


G.add_edges_from([
                    ('f', 'g'), 
                    ('f', 'h'), 
                    ('g', 'h'), 
                    ('f', 'i'), 
                    ('g', 'i'), 
                ])

# val_map = {'A': 1.0, 'D': 0.5714285714285714, 'H': 0.0}

# values = [val_map.get(node, 0.25) for node in G.nodes()]

red_edges = list()
# [('A', 'C'), ('E', 'C')]


