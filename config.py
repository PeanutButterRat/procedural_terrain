﻿# config.py

def can_build(env, platform):
    return True

def configure(env):
    pass

def get_doc_path():
    return "doc_classes"

def get_doc_classes():
    return [
        "ProceduralTerrainParameters",
	"ProceduralTerrain",
    ]

def get_icons_path():
    return "icons"