from PIL import Image, ImageDraw, ImageFont
import time
import random, os, sys
from colorsys import hsv_to_rgb

from Character import Character 
from Joystick import Joystick

# 왼 위 오 아래
joystick = Joystick()

# 게임 시작 출력--------------------------------------------------------------------------
# 이미지 크기 설정
image_width, image_height = 240, 240
background_color = (0, 0, 0, 0)  # 배경색을 검정색으로 설정하거나 필요한 색상으로 변경

# 새 이미지 생성
image = Image.new("RGBA", (image_width, image_height), background_color)
draw = ImageDraw.Draw(image)

# 텍스트 설정
text = "GAME START!!"
text_color = (255, 255, 255)  # 텍스트 색상을 흰색으로 설정하거나 필요한 색상으로 변경
font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24)  # 폰트와 크기 설정

# 텍스트 크기 계산
text_bbox = draw.textbbox((0, 0), text, font=font)
text_width = text_bbox[2] - text_bbox[0]
text_height = text_bbox[3] - text_bbox[1]

# 텍스트를 이미지 중앙에 배치하기 위한 위치 계산
text_x = (image_width - text_width) // 2
text_y = (image_height - text_height) // 2

# 텍스트를 이미지에 그리기
draw.text((text_x, text_y), text, fill=text_color, font=font)

# 이미지를 화면에 표시
joystick.disp.image(image)
    
time.sleep(2)
 # 게임 시작 출력 완----------------------------------------------------------------------   
# 하트개수 출력 --------------------------------------------------------------------------
def H(cnt):
    # 이미지 크기 설정
    image_width, image_height = 240, 240
    background_color = (0, 0, 0, 100)  # 배경색 검정색

    # 새 이미지 생성
    image = Image.new("RGBA", (image_width, image_height), background_color)
    draw = ImageDraw.Draw(image)

    # 텍스트 설정
    text = "your heart: "+str(5-cnt)
    text_color = (255, 0, 0)  # 텍스트 색 빨간색
    font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24)  # 폰트와 크기 설정

    # 텍스트 크기 계산
    text_bbox = draw.textbbox((0, 0), text, font=font)
    text_width = text_bbox[2] - text_bbox[0]
    text_height = text_bbox[3] - text_bbox[1]

    # 텍스트를 이미지 중앙에 배치하기 위한 위치 계산
    text_x = (image_width - text_width) // 2
    text_y = (image_height - text_height) // 2

    # 텍스트를 이미지에 그리기
    draw.text((text_x, text_y), text, fill=text_color, font=font)

    # 현재 화면 이미지와 합성
    composed_image = Image.alpha_composite(cropped_image.convert("RGBA"), image)

    # 이미지를 화면에 표시
    joystick.disp.image(composed_image)
    time.sleep(2)
# --------------------------------------------------------------------

my_stone = Character(joystick.width, joystick.height)

# 이미지 파일 경로
image_path = 'image/background.png'
back = Image.open(image_path)

# 초기 상태 설정
speed=[1,2,3] # 단계별 
P = 0
scroll_speed = speed[P]
current_position = 0

obstacle_positions = []  # 장애물의 위치를 저장할 리스트

# 사각형들의 초기 위치
rectangle_positions = [(0, 100+100 * i) for i in range(23)]  # 100 간격으로 23개의 사각형이 배치

# 각 사각형의 뚫린 부분을 나타내는 리스트
holes = [random.randint(0, 180) for _ in range(len(rectangle_positions))]  # 240 -60=180

# 장애물의 위치 설정
for i, position in enumerate(rectangle_positions):
    if i == 0: continue
    obstacle_x = random.randint(position[0], position[0] + 180)  # 랜덤한 x좌표 설정
    while holes[i]-15 <= obstacle_x <= holes[i] + 60: # 홀 피해서
        obstacle_x = random.randint(position[0], position[0] + 180)  # 랜덤한 x좌표 설정
    obstacle_y = position[1] - 40  # 흰 사각형 윗면보다 위에 장애물 배치
    obstacle_positions.append((obstacle_x, obstacle_y))

def check_collision(obstacle_x, obstacle_y):
    # 돌의 모서리좌표
    stone_points = [
        (my_stone.position[0], my_stone.position[1]), 
        (my_stone.position[0] + 32, my_stone.position[1]),  
        (my_stone.position[0], my_stone.position[1] + 29), 
        (my_stone.position[0] + 32, my_stone.position[1] + 29), 
    ]

    # 장애물의좌표
    obstacle_points = [
        (obstacle_x, obstacle_y),
        (obstacle_x + obstacle_image.width, obstacle_y + obstacle_image.height)
    ]

    # 충돌 계산
    for point in stone_points:
        if obstacle_points[0][0] <= point[0] <= obstacle_points[1][0] and \
           obstacle_points[0][1] <= point[1] <= obstacle_points[1][1]:
            return True  # 충돌 발생

    return False  # 충돌 없음
cnt = 0

# 이미지의 높이 가져오기
image_height = back.height  # 이미지의 높이를 가져옵니다.

# 캐릭터 이미지의 동근모습그대록
mask = my_stone.appearances[my_stone.image_index].split()[3]
    
result = 0
while True: # 게임 시작---------------------------------------------------------------
    command = None
    if not joystick.button_L.value:  # left pressed
        command = 'left_pressed'
    elif not joystick.button_R.value:  # right pressed
        command = 'right_pressed'
    else:
        command = None
    my_stone.move(command)

    # 이미지가 끝까지 스크롤되면 스크롤 중지
    if current_position > image_height - joystick.height:  # > 2400 - 240
        scroll_speed = 0
        
    # 돌이 아래로 떨어지지 않게 유지
    elif my_stone.position[1] + 29 > joystick.height / 2 + 40:  # 돌이 디스플레이 중앙보다 아래에 위치하면
        scroll_speed = 10  # 배경의 스크롤 속도를 10으로 변경
        my_stone.position[1] -= 10  # 돌을 
        my_stone.position[3] -= 10  # 위로 조금 이동
    else:
        scroll_speed = speed[P]  # 그 외에는 기본 스크롤 속도로 유지

    # 돌이 위로 올라가면 사망
    if my_stone.position[1] < 0:
        result = 0
        break
    # 돌이 집 도착하면 성공
    if my_stone.position[1] + 45 > joystick.height:
        result = 1
        break    
    # 돌이 벽 너머로 순간이동
    if my_stone.position[2] < 0:
        my_stone.position[0] = joystick.width
        my_stone.position[2] = joystick.width + my_stone.appearance.width
    elif my_stone.position[0] >joystick.width:
        my_stone.position[0] = - my_stone.appearance.width
        my_stone.position[2] = 0
        
    # 배경이미지 스크롤
    current_position += scroll_speed
    
    if current_position > 1600:
        P = 2
    elif current_position > 800:
        P = 1

    # 사각형들 돌면서 흰직사각형에 닿았는지
    hit_white_rectangle = False
    for i, position in enumerate(rectangle_positions):
        if position[1] - current_position <= my_stone.position[1] + 29 <= position[1] + 12 - current_position:
            if not (holes[i] <= my_stone.position[0] <= holes[i] + 28):
                hit_white_rectangle = True
                break

    # 흰직사각형에 닿지 않으면 아래로 이동/ 아니면 못 함
    if not hit_white_rectangle: 
        my_stone.position[1] += 10  # 아래로 이동
        my_stone.position[3] += 10  # 아래로 이동
    if hit_white_rectangle:
        my_stone.position[1] -= scroll_speed  # 아래로 이동 못 함
        my_stone.position[3] -= scroll_speed  # 아래로 이동 못 함

    
    # 이미지 크롭/ 화면에 표시
    cropped_image = back.crop((0, current_position, joystick.width, joystick.height + current_position))
    display_image = cropped_image.copy()  # 배경 이미지 복사
    my_draw = ImageDraw.Draw(display_image)  # 이미지 위에 그리기 도구 생성

    # 이미지 위에 캐릭터 그리기
    display_image.paste(my_stone.appearances[my_stone.image_index], tuple(my_stone.position),mask)

    # 장애물 이미지
    obstacle_image = Image.open('image/obstacle.png')

    # 이미지 위에 장애물 그리기
    for obstacle_pos in obstacle_positions:
        obstacle_x, obstacle_y = obstacle_pos  # 각 장애물 좌표
        obstacle_y -= current_position  # 스크롤에 -> y 좌표
        display_image.paste(obstacle_image, (obstacle_x, obstacle_y))

    # 사각형들 그리기
    for i, position in enumerate(rectangle_positions):
        rect_position = (position[0], position[1] - current_position)  # 스크롤에 맞게
        hole_start = holes[i]
        hole_end = hole_start + 60  # 뚫린 부분은 가로 60

        my_draw.rectangle(
            [(rect_position[0], rect_position[1]), (hole_start, rect_position[1] + 12)],  # 왼쪽 사각
            fill="white"
        )
        my_draw.rectangle(
            [(hole_end, rect_position[1]), (rect_position[0] + 240, rect_position[1] + 12)],  # 오른쪽 사각
            fill="white"
        )

    # 충돌
    for obstacle_pos in obstacle_positions:
        obstacle_x, obstacle_y = obstacle_pos  # 장애물 좌표
        obstacle_y -= current_position  # 스크롤에 맞게 -> y 좌표
        display_image.paste(obstacle_image, (obstacle_x, obstacle_y))

        # 장애물과 돌 충돌 함수ㄱㄱ
        if check_collision(obstacle_x, obstacle_y):
            cnt += 1  # 충돌 횟수
            if cnt >= 5: # 목숨 다섯개
                result = 3
                break
            H(cnt)
            print(cnt)
            my_stone.position[0] += 20
            my_stone.position[2] += 20
            
    if result == 3:
        result = 0
        break
    # 디스플레이에 이미지 표시
    joystick.disp.image(display_image)



# 게임 실패 했다. --------------------------------------------------------------------------
if result == 0:
    time.sleep(2)
    
    # 이미지 크기 설정
    image_width, image_height = 240, 240
    background_color = (0, 0, 0, 100)  # 배경색 검정색

    # 새 이미지 생성
    image = Image.new("RGBA", (image_width, image_height), background_color)
    draw = ImageDraw.Draw(image)

    # 텍스트 설정
    text_gameover = "GAME OVER!!"
    text_color_gameover = (255, 0, 0)  # 텍스트 색 빨강색
    font_gameover = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24)  # 폰트와 크기 설정

    # 텍스트 크기 계산
    text_bbox_gameover = draw.textbbox((0, 0), text_gameover, font=font_gameover)
    text_width_gameover = text_bbox_gameover[2] - text_bbox_gameover[0]
    text_height_gameover = text_bbox_gameover[3] - text_bbox_gameover[1]

    # 텍스트를 이미지 중앙에 배치하기 위한 위치 계산
    text_x_gameover = (image_width - text_width_gameover) // 2
    text_y_gameover = (image_height - text_height_gameover) // 2

    # 텍스트를 이미지에 그리기
    draw.text((text_x_gameover, text_y_gameover), text_gameover, fill=text_color_gameover, font=font_gameover)
    
    # 텍스트 설정 - "RESTART ->"
    text_restart = "RESTART ->"
    text_color_restart = (255, 255, 255)  # 텍스트 색 흰색
    font_restart = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 18)  # 폰트와 크기 설정

    # 텍스트 크기 계산 - "RESTART ->"
    text_bbox_restart = draw.textbbox((0, 0), text_restart, font=font_restart)
    text_width_restart = text_bbox_restart[2] - text_bbox_restart[0]
    text_height_restart = text_bbox_restart[3] - text_bbox_restart[1]

    # 텍스트를 이미지 중앙에 배치하기 위한 위치 계산 - "RESTART ->"
    text_x_restart = (image_width - text_width_restart) // 2
    text_y_restart = text_y_gameover + text_height_gameover + 40  # "GAME OVER!!" 아래에 여백 추가

    # 텍스트를 이미지에 그리기 - "RESTART ->"
    draw.text((text_x_restart, text_y_restart), text_restart, fill=text_color_restart, font=font_restart)

    # 현재 화면 이미지와 합성
    composed_image = Image.alpha_composite(cropped_image.convert("RGBA"), image)

    # 이미지를 화면에 표시
    joystick.disp.image(composed_image)

    # # 현재 화면 이미지와 합성
    # composed_image = Image.alpha_composite(cropped_image.convert("RGBA"), image)

    # # 이미지를 화면에 표시
    # joystick.disp.image(composed_image)
    
    time.sleep(2) # 딜레이 주기
    while True:
        if not joystick.button_A.value: # A pressed -> restart
            os.execv(sys.executable, ['python'] + sys.argv)
        
# 게임 클리어 했다. -------------------------------------------------------------------------
if result == 1:
    time.sleep(2) # 딜레이 주기
    # 이미지 크기 설정
    image_width, image_height = 240, 240
    background_color = (0, 0, 0, 100)  # 배경색 검정색

    # 새 이미지 생성
    image = Image.new("RGBA", (image_width, image_height), background_color)
    draw = ImageDraw.Draw(image)

    # 텍스트 설정
    text = "GAME CLEAR!!"
    text_color = (255, 255, 255)  # 텍스트 색 빨간색
    font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24)  # 폰트와 크기 설정

    # 텍스트 크기 계산
    text_bbox = draw.textbbox((0, 0), text, font=font)
    text_width = text_bbox[2] - text_bbox[0]
    text_height = text_bbox[3] - text_bbox[1]

    # 텍스트를 이미지 중앙에 배치하기 위한 위치 계산
    text_x = (image_width - text_width) // 2
    text_y = (image_height - text_height) // 2

    # 텍스트를 이미지에 그리기
    draw.text((text_x, text_y), text, fill=text_color, font=font)

    # 현재 화면 이미지와 합성
    composed_image = Image.alpha_composite(cropped_image.convert("RGBA"), image)

    # 이미지를 화면에 표시
    joystick.disp.image(composed_image)