<template>
  <Card class="control-card">
    <template #title>
      <span class="card-title">任务控制</span>
    </template>
    <template #content>
      <div class="field">
        <label>任务选择（可多选，按顺序执行）</label>
        <div class="task-list">
          <div class="task-item" v-for="task in tasks" :key="task.name">
            <Checkbox
              :inputId="`task-${task.name}`"
              :value="task.name"
              v-model="selectedTasks"
              @change="emit('tasks-change')"
            />
            <label class="task-label" :for="`task-${task.name}`">{{ task.label }}</label>
            <Tag
              v-if="taskStatuses[task.name]"
              class="task-status"
              :value="taskStatuses[task.name].label"
              :severity="taskStatuses[task.name].severity"
            />
            <Tag v-if="task.queue?.must_last" value="队尾" severity="secondary" />
            <Tag v-if="task.queue?.is_looping" value="循环" severity="contrast" />
          </div>
          <div v-if="tasks.length === 0" class="task-empty">等待任务列表...</div>
        </div>
      </div>

      <div class="queue-tip" v-if="queuePreview.length">
        当前队列：{{ queuePreview.join(' → ') }}
      </div>

      <div class="button-row">
        <Button
          :label="executionBusy ? '执行中...' : '开始执行'"
          :loading="executionBusy"
          @click="emit('start-queue')"
        />
        <Button label="停止当前任务" severity="secondary" outlined @click="emit('stop-task')" />
      </div>

      <div class="helper">队列顺序和任务限制由后端配置驱动。</div>
    </template>
  </Card>
</template>

<script setup lang="ts">
import Button from 'primevue/button'
import Card from 'primevue/card'
import Checkbox from 'primevue/checkbox'
import Tag from 'primevue/tag'
import type { TaskMeta, TaskStatusTag } from './types'

defineProps<{
  tasks: TaskMeta[]
  queuePreview: string[]
  executionBusy: boolean
  taskStatuses: Record<string, TaskStatusTag>
}>()

const selectedTasks = defineModel<string[]>('selectedTasks', { required: true })

const emit = defineEmits<{
  (event: 'tasks-change'): void
  (event: 'start-queue'): void
  (event: 'stop-task'): void
}>()
</script>

<style scoped>
.field {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.field label {
  font-size: 0.875rem;
  font-weight: 500;
  color: var(--text-color-secondary);
}

.task-list {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 12px;
}

.task-item {
  display: flex;
  align-items: center;
  gap: 8px;
}

.task-label {
  cursor: pointer;
  flex: 1;
}

.task-status {
  margin-left: 4px;
}

.task-empty {
  grid-column: 1 / -1;
  text-align: center;
  padding: 20px;
  color: var(--text-color-secondary);
  font-size: 0.875rem;
}

.button-row {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
  margin-top: 16px;
}

.helper {
  font-size: 0.875rem;
  color: var(--text-color-secondary);
  margin-top: 12px;
}

.queue-tip {
  padding: 12px;
  background: var(--surface-100);
  border-radius: 6px;
  font-size: 0.875rem;
  color: var(--text-color-secondary);
  margin: 12px 0;
}

@media (max-width: 960px) {
  .task-list {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 600px) {
  .task-list {
    grid-template-columns: 1fr;
  }
}
</style>
