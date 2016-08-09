#-*- coding: utf-8 -*-

from src.shell.command.i_command import ICommand

class MemMap(ICommand):
    """
        Draw an HTML map of memory usage

    """

    WINDOW_SIZE = 16000
    HTML_WIDTH = 64
    
    def __init__(self, data, tpl_file, log=None, verbose=False):
        super(MemMap, self).__init__(log)
        self.__data == data
        self.__html = ["", ""]
        self.__tpl_file = tpl_file

    def run(self, *args, **kwargs):
        self.render(args, kwargs)

    def __find_point_of_interest(self):
        self.log("finding points of interest")
        # Search for max density area
        senti_low = 0
        senti_high = 0
        while self.__data[senti_high][0] - self.__data[senti_low][0] < MemMap.WINDOW_SIZE:
            senti_high += 1
        nmax = senti_high - senti_low
        n = nmax
        while senti_high < len(self.__data) - 1:
            senti_high += 1
            n += 1
            while self.__data[senti_high][0] - self.__data[senti_low][0] > MemMap.WINDOW_SIZE:
                senti_low += 1
                n -= 1
            if n > nmax:
                nmax = n
                senti_low_best = senti_low
        return senti_low_best, senti_low_best + nmax

    def render(self):
        self.log("rendering html view")
        senti = 0
        WDW = self.__data[-1][0] - self.__data[0][0]
        n = 0
        IDX_MIN, IDX_MAX = self.__find_point_of_interest() 
        # Keep addresses aligned
        ADDR_MIN = self.__data[IDX_MIN][0] - (self.__data[IDX_MIN][0] % MemMap.HTML_WIDTH)
        ADDR_MAX = self.__data[IDX_MAX][0] - (self.__data[IDX_MAX][0] % MemMap.HTML_WIDTH) - 1
        ALLOC_MIN = min([d[1] for d in self.__data[IDX_MIN:IDX_MAX]])
        ALLOC_MAX = max([d[1] for d in self.__data[IDX_MIN:IDX_MAX]])
        FREE_MIN = min([d[2] for d in self.__data[IDX_MIN:IDX_MAX]])
        FREE_MAX = max([d[2] for d in self.__data[IDX_MIN:IDX_MAX]])
        NB_ALLOC_LEVEL = 9
        NB_FREE_LEVEL = 3

        senti = IDX_MIN

        # ALLOC Header : first row contains value of first considered addr
        self.__html[0] = "<tr><td class=\"hdr-addr\" colspan=\"{1}\" class=\"hdr-addr\">{0:06x}</td></tr>".format(ADDR_MIN, MemMap.HTML_WIDTH)

        for i in xrange(ADDR_MIN, ADDR_MAX + 1):
            if self.__data[senti][0] == i:
                d = self.__data[senti]
                senti += 1
            else:
                d = (i, 0, 0)
            if i % MemMap.HTML_WIDTH == 0:
                self.__html[0] += "<tr>"
                self.__html[1] += "<tr>"
            self.__html[0] += "<td data-addr={0} data-intensity={1} data-alloc={2}></td>\n".format(d[0], ((-ALLOC_MIN + d[1]) * NB_ALLOC_LEVEL) / ALLOC_MAX, 0)
            if i % MemMap.HTML_WIDTH == MemMap.HTML_WIDTH - 1:
                self.__html[0] += "</tr>"
                self.__html[1] += "</tr>"
        if i % MemMap.HTML_WIDTH != MemMap.HTML_WIDTH - 1:
            self.__html[0] += "</tr>"
        return

    def get_html(self):
        return self.__html[0], self.__html[1]

    def export(self, output_file):
        self.log("exporting to html file")
        with open(self.__tpl_file, "r") as f:
            content = f.read()

        soup = BeautifulSoup(content, 'html.parser')

        memap_alloc = soup.select("#alloc > tbody")[0]
        memap_free = soup.select("#free > tbody")[0]

        alloc_content = BeautifulSoup(self.__html[0], "lxml")
        free_content = BeautifulSoup(self.__html[1], "lxml")

        memap_alloc.append(alloc_content)
        memap_free.append(free_content)
        
        with open(output_file, "w") as f:
            f.write(soup.prettify())

