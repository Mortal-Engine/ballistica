// Released under the MIT License. See LICENSE for details.

#include "ballistica/audio/audio_source.h"

#include "ballistica/assets/data/sound_data.h"
#include "ballistica/audio/audio.h"
#include "ballistica/audio/audio_server.h"
#include "ballistica/math/vector3f.h"

namespace ballistica {

AudioSource::AudioSource(int id_in) : id_(id_in) {}

AudioSource::~AudioSource() { assert(client_queue_size_ == 0); }

void AudioSource::MakeAvailable(uint32_t play_id_new) {
  assert(AudioServer::source_id_from_play_id(play_id_new) == id_);
  assert(client_queue_size_ == 0);
  assert(locked());
  play_id_ = play_id_new;
  assert(!available_);
  assert(g_audio);
  g_audio->MakeSourceAvailable(this);
  available_ = true;
}

void AudioSource::SetIsMusic(bool val) {
  assert(g_audio_server);
  assert(client_queue_size_ > 0);
  g_audio_server->PushSourceSetIsMusicCall(play_id_, val);
}

void AudioSource::SetPositional(bool val) {
  assert(g_audio_server);
  assert(client_queue_size_ > 0);
  g_audio_server->PushSourceSetPositionalCall(play_id_, val);
}

void AudioSource::SetPosition(float x, float y, float z) {
  assert(g_audio_server);
  assert(client_queue_size_ > 0);
#if BA_DEBUG_BUILD
  if (std::isnan(x) || std::isnan(y) || std::isnan(z)) {
    Log(LogLevel::kError, "Got nan value in AudioSource::SetPosition.");
  }
#endif
  g_audio_server->PushSourceSetPositionCall(play_id_, Vector3f(x, y, z));
}

void AudioSource::SetGain(float val) {
  assert(g_audio_server);
  assert(client_queue_size_ > 0);
  g_audio_server->PushSourceSetGainCall(play_id_, val);
}

void AudioSource::SetFade(float val) {
  assert(g_audio_server);
  assert(client_queue_size_ > 0);
  g_audio_server->PushSourceSetFadeCall(play_id_, val);
}

void AudioSource::SetLooping(bool val) {
  assert(g_audio_server);
  assert(client_queue_size_ > 0);
  g_audio_server->PushSourceSetLoopingCall(play_id_, val);
}

auto AudioSource::Play(SoundData* ptr_in) -> uint32_t {
  assert(ptr_in);
  assert(g_audio_server);
  assert(client_queue_size_ > 0);

  // allocate a new reference to this guy and pass it along
  // to the thread... (these refs can't be created or destroyed
  // or have their ref-counts changed outside the main thread...)
  // the thread will then send back this allocated ptr when it's done
  // with it for the main thread to destroy.

  ptr_in->UpdatePlayTime();
  auto ptr = new Object::Ref<SoundData>(ptr_in);
  g_audio_server->PushSourcePlayCall(play_id_, ptr);
  return play_id_;
}

void AudioSource::Stop() {
  assert(g_audio_server);
  assert(client_queue_size_ > 0);
  g_audio_server->PushSourceStopCall(play_id_);
}

void AudioSource::End() {
  assert(client_queue_size_ > 0);
  // send the thread a "this source is potentially free now" message
  assert(g_audio_server);
  g_audio_server->PushSourceEndCall(play_id_);
  Unlock();
}

void AudioSource::Lock(int debug_id) {
  BA_DEBUG_FUNCTION_TIMER_BEGIN();
  mutex_.lock();
#if BA_DEBUG_BUILD
  last_lock_time_ = GetRealTime();
  lock_debug_id_ = debug_id;
  locked_ = true;
#endif
  BA_DEBUG_FUNCTION_TIMER_END_THREAD(20);
}

auto AudioSource::TryLock(int debug_id) -> bool {
  bool locked = mutex_.try_lock();
#if (BA_DEBUG_BUILD || BA_TEST_BUILD)
  if (locked) {
    locked_ = true;
    last_lock_time_ = GetRealTime();
    lock_debug_id_ = debug_id;
  }
#endif
  return locked;
}

void AudioSource::Unlock() {
  BA_DEBUG_FUNCTION_TIMER_BEGIN();
  mutex_.unlock();
  BA_DEBUG_FUNCTION_TIMER_END_THREAD(20);
#if BA_DEBUG_BUILD || BA_TEST_BUILD
  locked_ = false;
#endif
}

}  // namespace ballistica
