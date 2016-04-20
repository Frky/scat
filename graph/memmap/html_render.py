
from bs4 import BeautifulSoup
from random import randint

def generate_random_access(N):
    data = dict()
    A_MIN = 0
    A_MAX = 10000
    R_MIN = 0
    R_MAX = 100
    W_MIN = 0
    W_MAX = 100
    for i in xrange(N):
        addr = randint(A_MIN, A_MAX)
        while addr in data.keys():
            addr = randint(A_MIN, A_MAX)
        data[addr] = (randint(R_MIN, R_MAX), randint(W_MIN, W_MAX))
    return sorted([(a, r, w) for a, (r, w) in data.items()], key=lambda a:a[0])

def render(data):
    a_min = data[0][0]
    a_max = data[-1][0]
    r_min = min([d[1] for d in data])
    r_max = max([d[1] for d in data])
    w_min = min([d[2] for d in data])
    w_max = max([d[2] for d in data])
    i_min = min([d[1] + d[2] for d in data])
    i_max = max([d[1] + d[2] for d in data])
    I_LEVEL = 6
    html = ""
    WIDTH = 16
    senti = 0
    a_min = a_min - (a_min % WIDTH)
    for i in xrange(a_min, a_max + 1):
        if data[senti][0] == i:
            d = data[senti]
            senti += 1
        else:
            d = (i, 0, 0)
        if i % WIDTH == 0:
            html += "<tr>\n"
            html += "<td class=\"hdr-addr\">{0:06x}</td>".format(i)
        html += "<td data-addr={0} data-intensity={1} data-read={2} data-write={3}></td>\n".format(d[0], ((-i_min + d[1] + d[2]) * I_LEVEL) / i_max, d[1], d[2])
        if i % WIDTH == WIDTH - 1:
            html += "</tr>\n"
    if i % WIDTH != WIDTH - 1:
        html += "</tr>\n"
    return html


with open("template.html", "r") as f:
    content = f.read()

soup = BeautifulSoup(content, 'html.parser')

memap = soup.select("#memory-map > tbody")[0]
data_content = BeautifulSoup(render(generate_random_access(1000)))
memap.append(data_content)

with open("mem_map.html", "w") as f:
    f.write(soup.prettify())


