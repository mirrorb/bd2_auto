<template>
  <div class="process-manager">
    <div content>
      <div class="input-area">
        <input v-model="inputText" @keyup.enter="sendInput" />
        <button @click="sendInput">发送</button>
      </div>
      <div class="output-area">
        <div v-for="(output, index) in outputs" :key="index">
          {{ output }}
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue'
import { listen, UnlistenFn } from '@tauri-apps/api/event'

const inputText = ref('{"command": "START_TASK", "task_name": "hello_task"}')
const outputs = ref<string[]>([])
let unlisten: UnlistenFn | null = null
import { invoke } from "@tauri-apps/api/core";

const sendInput = async () => {
  if (!inputText.value.trim()) return
  try {
    await invoke('core_input', { input: inputText.value })
    inputText.value = ''
  } catch (e) {
    console.error('发送输入失败:', e)
  }
}

const eventHandler = async () => {
  unlisten = await listen('core-output', (event) => {
    outputs.value.push(event.payload as string)
  })
}

onMounted(() => {
  eventHandler()
})

onUnmounted(() => {
  if (unlisten) {
    unlisten()
  }
})
</script>
