from PIL import Image, ImageDraw

def create_icon(name, draw_func):
    img = Image.new('RGBA', (256, 256), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw_func(draw)
    img.save(name)

# Play (Triangle)
create_icon('PlayButton.png', lambda d: d.polygon([(64, 48), (64, 208), (208, 128)], fill=(200, 200, 200, 255)))

# Pause (Two vertical bars)
create_icon('PauseButton.png', lambda d: [d.rectangle([64, 48, 104, 208], fill=(200, 200, 200, 255)), d.rectangle([152, 48, 192, 208], fill=(200, 200, 200, 255))])

# Stop (Square)
create_icon('StopButton.png', lambda d: d.rectangle([64, 64, 192, 192], fill=(200, 200, 200, 255)))

# Step (Two triangles pointing right)
create_icon('StepButton.png', lambda d: [
    d.polygon([(40, 64), (40, 192), (120, 128)], fill=(200, 200, 200, 255)),
    d.polygon([(120, 64), (120, 192), (200, 128)], fill=(200, 200, 200, 255))
])

# Add (Plus)
create_icon('AddButton.png', lambda d: [d.rectangle([112, 32, 144, 224], fill=(200, 200, 200, 255)), d.rectangle([32, 112, 224, 144], fill=(200, 200, 200, 255))])

