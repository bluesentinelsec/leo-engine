#ifndef LEO_AUDIO_H
#define LEO_AUDIO_H

namespace engine
{

class VFS;

class Sound
{
  public:
    Sound() noexcept;
    ~Sound();
    Sound(Sound &&other) noexcept;
    Sound &operator=(Sound &&other) noexcept;

    Sound(const Sound &) = delete;
    Sound &operator=(const Sound &) = delete;

    static Sound LoadFromVfs(VFS &vfs, const char *vfs_path);

    bool IsReady() const noexcept;
    void Reset() noexcept;

    void Play();
    void Pause();
    void Stop();
    bool IsPlaying() const noexcept;

    void SetLooping(bool looping);
    void SetVolume(float volume);
    void SetPitch(float pitch);

  private:
    void *handle;
    bool paused;
};

class Music
{
  public:
    Music() noexcept;
    ~Music();
    Music(Music &&other) noexcept;
    Music &operator=(Music &&other) noexcept;

    Music(const Music &) = delete;
    Music &operator=(const Music &) = delete;

    static Music LoadFromVfs(VFS &vfs, const char *vfs_path);

    bool IsReady() const noexcept;
    void Reset() noexcept;

    void Play();
    void Pause();
    void Stop();
    bool IsPlaying() const noexcept;

    void SetLooping(bool looping);
    void SetVolume(float volume);
    void SetPitch(float pitch);

  private:
    void *handle;
    bool paused;
};

} // namespace engine

#endif // LEO_AUDIO_H
