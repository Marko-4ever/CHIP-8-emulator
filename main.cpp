#include <cstdint>
#include <raylib.h>
import std;

struct Chip8
{
  std::array<std::uint8_t, 4096> memory;
  std::array<std::uint8_t, 16> V;
  std::uint16_t I;
  std::uint16_t PC;
  std::array<std::uint16_t, 16> stack;
  std::uint8_t sp;
  std::uint8_t delay_timer;
  std::uint8_t sound_timer;

  std::array<bool, 64 * 32> display{};
  std::array<bool, 16> keypad{false};

  Chip8()
      : memory{}, V{}, I(0), PC(0x200), stack{}, sp(0), delay_timer(0),
        sound_timer(0)
  {
    std::array<std::uint8_t, 80> fontset = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    for (std::size_t i = 0; i < fontset.size(); ++i)
    {
      memory[0x50 + i] = fontset[i];
    }
  }

  bool loadROM(const std::string &filepath)
  {
    // 1. Open the file in binary mode and start at the end (ate) to find size
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);

    // 2. Check if the file actually opened
    if (!file.is_open())
    {
      std::cerr << "[-] Failed to open ROM file: " << filepath << "\n";
      return false;
    }

    // 3. Get the file size in bytes using tellg()
    std::streamsize size = file.tellg();

    // 4. Validate that the ROM fits in RAM (Total RAM is 4096 bytes, starting
    // at 0x200)
    if (size > (4096 - 0x200))
    {
      std::cerr << "[-] Error: ROM is too large (" << size
                << " bytes) to fit in RAM!\n";
      file.close();
      return false;
    }

    // 5. Move the read pointer back to the beginning of the file
    file.seekg(0, std::ios::beg);

    // 6. Read the file bytes directly into your memory array starting at index
    // 0x200 (decimal 505)
    if (file.read(reinterpret_cast<char *>(&memory[0x200]), size))
    {
      std::cout << "[+] Successfully loaded ROM: " << filepath << " (" << size
                << " bytes)\n";
    }
    else
    {
      std::cerr << "[-] Error: Failed to read ROM into memory.\n";
      file.close();
      return false;
    }

    // 7. Close the file stream
    file.close();
    return true;
  }

  void printDebugState()
  {
    std::println("PC {:#X}", PC);
    std::println("I {:#X}", I);
    std::println("sp {:#X}, delay {:#X}, sound {:#X}", sp, delay_timer,
                 sound_timer);

    for (std::size_t i = 0; i < V.size(); i++)
    {
      std::println("V{:X}: {:#04X}", i, V[i]);
    }

    for (int i = 0; i < 64; i++)
    {
      std::uint16_t addr = 0x200 + i;
      std::print("{:#04X} ", memory[addr]);
      if ((i + 1) % 16 == 0)
      {
        std::println("");
      }
    }
  }

  void emulateCycle()
  {
    std::uint16_t opcode = (memory[PC] << 8) | memory[PC + 1];
    PC += 2;

    std::uint16_t family = (opcode & 0xF000) >> 12;

    switch (family)
    {
    case 0x0:
    {
      std::uint8_t subcode = opcode & 0x00FF;
      switch (subcode)
      {
      case 0xE0:
        display.fill(false);
        break;
      case 0xEE:
        --sp;
        PC = stack[sp];
        break;
      }

      break;
    }
    case 0x2:
      stack[sp] = PC;
      ++sp;
      PC = opcode & 0x0FFF;
      break;
    case 0x1:
      PC = opcode & 0x0FFF;
      break;
    case 0x6:
    {
      std::uint8_t X = (opcode & 0x0F00) >> 8;
      std::uint8_t NN = opcode & 0x00FF;
      V[X] = NN;
      break;
    }
    case 0x7:
    {
      std::uint8_t X = (opcode & 0x0F00) >> 8;
      std::uint8_t NN = opcode & 0x00FF;
      V[X] += NN;
      break;
    }
    case 0x8:
    {
      std::uint8_t sub_opcode = opcode & 0x000F;

      switch (sub_opcode)
      {
      case 0x0:
      {
        std::uint8_t X = (opcode & 0x0F00) >> 8;
        std::uint8_t Y = (opcode & 0x00F0) >> 4;
        V[X] = V[Y];
        break;
      }
      case 0x1:
      {
        std::uint8_t X = (opcode & 0x0F00) >> 8;
        std::uint8_t Y = (opcode & 0x00F0) >> 4;
        V[X] |= V[Y];
        break;
      }
      case 0x2:
      {
        std::uint8_t X = (opcode & 0x0F00) >> 8;
        std::uint8_t Y = (opcode & 0x00F0) >> 4;
        V[X] &= V[Y];
        break;
      }
      case 0x3:
      {
        std::uint8_t X = (opcode & 0x0F00) >> 8;
        std::uint8_t Y = (opcode & 0x00F0) >> 4;
        V[X] ^= V[Y];
        break;
      }
      case 0x4:
      {
        std::uint8_t X = (opcode & 0x0F00) >> 8;
        std::uint8_t Y = (opcode & 0x00F0) >> 4;
        int sum = V[X] + V[Y];
        if (sum > 255)
        {
          V[0xF] = 1;
        }
        else
        {
          V[0xF] = 0;
        }

        V[X] = (std::uint8_t)(sum & 0xFF);
        break;
      }
      case 0x5:
      {
        std::uint8_t X = (opcode & 0x0F00) >> 8;
        std::uint8_t Y = (opcode & 0x00F0) >> 4;

        if (V[X] >= V[Y])
        {
          V[0xF] = 1;
        }
        else
        {
          V[0XF] = 0;
        }

        V[X] -= V[Y];
        break;
      }
      case 0x6:
      {
        std::uint8_t X = (opcode & 0x0F00) >> 8;

        V[0XF] = V[X] & 0x1;
        V[X] >>= 1;
        break;
      }
      case 0x7:
      {
        std::uint8_t X = (opcode & 0x0F00) >> 8;
        std::uint8_t Y = (opcode & 0x00F0) >> 4;

        if (V[Y] >= V[X])
        {
          V[0XF] = 1;
        }
        else
        {
          V[0XF] = 0;
        }

        V[X] = V[Y] - V[X];
        break;
      }
      case 0xE:
      {
        std::uint8_t X = (opcode & 0x0F00) >> 8;
        V[0XF] = (V[X] & 0x80) >> 7;
        V[X] <<= 1;
        break;
      }
      }

      break;
    }
    case 0x9:
    {
      std::uint8_t X = (opcode & 0x0F00) >> 8;
      std::uint8_t Y = (opcode & 0x00F0) >> 4;

      if (V[X] != V[Y])
      {
        PC += 2;
      }

      break;
    }
    case 0x3:
    {
      std::uint8_t X = (opcode & 0x0F00) >> 8;
      std::uint8_t NN = opcode & 0x00FF;

      if (V[X] == NN)
      {
        PC += 2;
      }

      break;
    }
    case 0x4:
    {
      std::uint8_t X = (opcode & 0x0F00) >> 8;
      std::uint8_t NN = opcode & 0x00FF;

      if (V[X] != NN)
      {
        PC += 2;
      }

      break;
    }
    case 0x5:
    {
      std::uint8_t X = (opcode & 0x0F00) >> 8;
      std::uint8_t Y = (opcode & 0x00F0) >> 4;

      if (V[X] == V[Y])
      {
        PC += 2;
      }

      break;
    }
    case 0xA:
    {
      std::uint16_t NNN = opcode & 0x0FFF;
      I = NNN;
      break;
    }
    case 0xB:
    {
      std::uint16_t NNN = opcode & 0x0FFF;
      PC = NNN + V[0];
      break;
    }
    case 0xC:
    {
      std::uint8_t X = (opcode & 0x0F00) >> 8;
      std::uint8_t NN = opcode & 0x00FF;

      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<int> dis(0, 255);

      V[X] = dis(gen) & NN;

      break;
    }
    case 0xD:
    {
      std::uint8_t X = (opcode & 0x0F00) >> 8;
      std::uint8_t Y = (opcode & 0x00F0) >> 4;
      std::uint8_t N = opcode & 0x000F;

      std::uint8_t xCoord = V[X] % 64;
      std::uint8_t yCoord = V[Y] % 32;
      V[0XF] = 0;

      for (int row = 0; row < N; ++row)
      {
        if ((yCoord + row) >= 32)
          break;
        std::uint8_t spriteByte = memory[I + row];

        for (int col = 0; col < 8; ++col)
        {
          if ((xCoord + col) >= 64)
            break;
          std::uint8_t spritePixel = spriteByte & (0x80 >> col);

          if (spritePixel != 0)
          {
            int currentX = xCoord + col;
            int currentY = yCoord + row;
            int index = currentY * 64 + currentX;

            if (display[index])
            {
              V[0XF] = 1;
            }

            display[index] = display[index] ^ true;
          }
        }
      }
      break;
    }
    case 0xF:
    {
      std::uint8_t X = (opcode & 0x0F00) >> 8;
      std::uint8_t subcode = opcode & 0x00FF;

      switch (subcode)
      {
      case 0x07:
        V[X] = delay_timer;
        break;
      case 0x15:
        delay_timer = V[X];
        break;
      case 0x18:
        sound_timer = V[X];
        break;
      case 0x1E:
        I += V[X];
        break;
      case 0x0A:
      {
        bool keyDetected = false;
        for (uint8_t i = 0; i < 16; ++i)
        {
          if (keypad[i])
          {
            V[X] = i;
            keyDetected = true;
            break;
          }
        }
        if (!keyDetected)
        {
          PC -= 2; // Wait by repeating this instruction
        }
        break;
      }
      case 0x29:
        I = 0x50 + (V[X] * 5);
        break;
      case 0x33:
      {
        std::uint8_t value = V[X];
        memory[I] = value / 100;
        memory[I + 1] = (value / 10) % 10;
        memory[I + 2] = value % 10;
        break;
      }
      case 0x55:
        for (std::uint8_t i = 0; i <= X; ++i)
        {
          memory[I + i] = V[i];
        }
        break;
      case 0x65:
        for (std::uint8_t i = 0; i <= X; ++i)
        {
          V[i] = memory[I + i];
        }
        break;
      }

      break;
    }
    case 0xE:
    {
      std::uint8_t X = (opcode & 0x0F00) >> 8;
      std::uint8_t subcode = opcode & 0x00FF;

      switch (subcode)
      {
      case 0x9E:
        if (keypad[V[X]] == true)
        {
          PC += 2;
        }
        break;
      case 0xA1:
        if (keypad[V[X]] == false)
        {
          PC += 2;
        }
        break;
      }

      break;
    }
    }
  }
};

void update_keypad(Chip8 &emu)
{
  emu.keypad[0x1] = IsKeyDown(KEY_ONE);
  emu.keypad[0x2] = IsKeyDown(KEY_TWO);
  emu.keypad[0x3] = IsKeyDown(KEY_THREE);
  emu.keypad[0xC] = IsKeyDown(KEY_FOUR);

  emu.keypad[0x4] = IsKeyDown(KEY_Q);
  emu.keypad[0x5] = IsKeyDown(KEY_W);
  emu.keypad[0x6] = IsKeyDown(KEY_E);
  emu.keypad[0xD] = IsKeyDown(KEY_R);

  emu.keypad[0x7] = IsKeyDown(KEY_A);
  emu.keypad[0x8] = IsKeyDown(KEY_S);
  emu.keypad[0x9] = IsKeyDown(KEY_D);
  emu.keypad[0xE] = IsKeyDown(KEY_F);

  emu.keypad[0xA] = IsKeyDown(KEY_Z);
  emu.keypad[0x0] = IsKeyDown(KEY_X);
  emu.keypad[0xB] = IsKeyDown(KEY_C);
  emu.keypad[0xF] = IsKeyDown(KEY_V);
}

int main(int argc, char *argv[])
{

  if (argc < 2)
  {
    std::println("Usage: ./chip8 <path_to_rom>");
    return 1;
  }

  Chip8 my_chip8;
  if (!my_chip8.loadROM(argv[1]))
  {
    return 1;
  }

  const int scale = 10;
  InitWindow(64 * scale, 32 * scale, "CHIP-8 emulator");
  InitAudioDevice();
  SetMasterVolume(0.5f);
  SetTargetFPS(60);

  Sound beep{};

  constexpr int sampleRate = 44100;
  constexpr float beepDuration = 0.12f;
  const int frameCount = static_cast<int>(sampleRate * beepDuration);

  std::vector<short> samples(frameCount);

  for (int i = 0; i < frameCount; ++i)
  {
    float t = static_cast<float>(i) / static_cast<float>(sampleRate);
    float tone = std::sin(2.0f * 3.14159265359f * 440.0f * t);
    float envelope = 1.0f - (t / beepDuration);
    if (envelope < 0.0f)
      envelope = 0.0f;

    samples[i] = static_cast<short>(tone * 20000.0f * envelope);
  }

  Wave beepWave = {};
  beepWave.frameCount = static_cast<unsigned int>(samples.size());
  beepWave.sampleRate = sampleRate;
  beepWave.sampleSize = 16;
  beepWave.channels = 1;
  beepWave.data = samples.data();

  beep = LoadSoundFromWave(beepWave);
  UnloadWave(beepWave);

  constexpr double cpuFrequency = 500.0;
  constexpr double timerFrequency = 60.0;
  double cpuAccumulator = 0.0;
  double timerAccumulator = 0.0;

  while (!WindowShouldClose())
  {
    double frameTime = GetFrameTime();
    update_keypad(my_chip8);

    cpuAccumulator += frameTime * cpuFrequency;
    while (cpuAccumulator >= 1.0)
    {
      my_chip8.emulateCycle();
      cpuAccumulator -= 1.0;
    }

    timerAccumulator += frameTime * timerFrequency;
    while (timerAccumulator >= 1.0)
    {
      if (my_chip8.delay_timer > 0)
      {
        --my_chip8.delay_timer;
      }
      if (my_chip8.sound_timer > 0)
      {
        if (!IsSoundPlaying(beep))
          PlaySound(beep);
        --my_chip8.sound_timer;
      }
      else
      {
        StopSound(beep);
      }
      timerAccumulator -= 1.0;
    }

    BeginDrawing();
    ClearBackground(BLACK);

    for (int y = 0; y < 32; ++y)
    {
      for (int x = 0; x < 64; ++x)
      {
        if (my_chip8.display[(y * 64) + x])
        {
          DrawRectangle(x * scale, y * scale, scale, scale, WHITE);
        }
      }
    }

    EndDrawing();
  }

  UnloadSound(beep);
  CloseAudioDevice();
  CloseWindow();
  return 0;
}