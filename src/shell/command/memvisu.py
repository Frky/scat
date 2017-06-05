#-*- coding: utf-8 -*-

import json 
import bottle

from src.shell.command.i_command import ICommand

class MemVisu(ICommand):
    """
        Retrieve couples

    """

    def __init__(self, config, json_path):
        super(MemVisu, self).__init__()
        self.__port = config["port"]
        self.__path = config["path"]
        self.__app = bottle.Bottle()
        with open(json_path, "r") as f:
            self.__data = json.loads(f.read())
        self.__route()

    def __route(self):
        self.__app.route('/', method="GET", callback=self.index)
        self.__app.route('/static/<filepath:path>', method="GET", callback=self.serve_static)

    def index(self):
        return bottle.template(self.__path["root"] + "index.html")

    def serve_static(self, filepath):
        return bottle.static_file(filepath, root=self.__path["static"])

    def run(self):
        self.__app.run(host='localhost', port=self.__port)
