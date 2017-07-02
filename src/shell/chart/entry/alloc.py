#-*- coding: utf-8 -*-

from .entry import Entry

class AllocEntry(Entry):

    def __init__(self, line, *args, **kwargs):
        l = line[:-1].split(":")
        self._pgm = l[0]
        self._alloc = l[1]
        if self._alloc != "None":
            self._alloc = ":".join(l[1:4])
            l.pop(2)
            l.pop(2)
        self._free = l[2]
        if self._free != "None":
            self._free = ":".join(l[2:5])
            l.pop(3)
            l.pop(3)
        self._calls = l[3:5]
        self._errors = l[5:7]
        self._online = float(l[7])
        self._offline = map(float, l[8:])
        super(AllocEntry, self).__init__(*args, **kwargs)

    @property
    def alloc(self):
        return self._alloc
        
    @property
    def free(self):
        return self._free
        
    @property
    def calls(self):
        return map(int, self._calls)
        
    @property
    def errors(self):
        return map(int, self._errors)
        
    @property
    def online(self):
        return self._online
        
    @property
    def offline(self):
        return self._offline
        
    @property
    def error_rate(self):
        if self.calls[0] * self.calls[1] == 0:
            return 0.0
        else:
            return (float(sum(self.errors)) / float(sum(self.calls)))

    @property
    def consistency(self):
        if self.calls[0] * self.calls[1] == 0:
            return 0.0
        else:
            return 1 - (float(sum(self.errors)) / float(sum(self.calls)))
        
