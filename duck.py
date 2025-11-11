import numpy as np
from PIL import Image, ImageDraw, ImageFont
import time
import random, os
from colorsys import hsv_to_rgb

class Character:
    def __init__(self, width, height):
        self.appearances = [Image.open('image/stone(1).png'),
                           Image.open('image/stone(2).png'),
                           Image.open('image/stone(3).png'),
                           Image.open('image/stone(4).png')]
        self.state = None
        self.position = [0,0,32,29] #초기위치
        self.image_index = 0

    def move(self, command = None):
        if command == None:
            self.state = None
        
        else:
            self.state = 'move'

            if command == 'left_pressed':
                self.position[0] -= 5
                self.image_index = (self.image_index - 1) % 4  # 3-2-1-0 순으로 반복
                self.appearance = self.appearances[self.image_index]
                self.position[2] -= 5
                
            elif command == 'right_pressed':
                self.position[0] += 5
                self.image_index = (self.image_index + 1) % 4  # 0-1-2-3 순으로 반복
                self.position[2] += 5
                self.appearance = self.appearances[self.image_index]