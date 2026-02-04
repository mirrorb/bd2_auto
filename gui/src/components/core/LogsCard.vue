<template>
  <Card class="logs-card">
    <template #title>
      <div class="logs-header">
        <span class="card-title">实时日志</span>
        <div class="logs-actions">
          <Select
            v-model="logLevelFilter"
            :options="logLevelOptions"
            optionLabel="label"
            optionValue="value"
            placeholder="选择等级"
          />
          <Button label="清空" severity="secondary" outlined @click="emit('clear')" />
        </div>
      </div>
    </template>
    <template #content>
      <div class="log-list">
        <div v-for="entry in entries" :key="entry.id" class="log-item" :class="entry.level">
          <div class="log-meta">
            <Tag :value="levelLabel(entry.level)" :severity="logLevelSeverity(entry.level)" />
            <span class="log-time">{{ entry.time }}</span>
            <span class="log-source">{{ entry.source }}</span>
          </div>
          <div class="log-message">{{ entry.message }}</div>
          <pre v-if="entry.detail" class="log-detail">{{ entry.detail }}</pre>
        </div>
        <div v-if="entries.length === 0" class="empty">暂无日志。</div>
      </div>
    </template>
  </Card>
</template>

<script setup lang="ts">
import Button from 'primevue/button'
import Card from 'primevue/card'
import Select from 'primevue/select'
import Tag from 'primevue/tag'
import type { LogEntry } from './types'

defineProps<{
  entries: LogEntry[]
  logLevelOptions: Array<{ label: string; value: 'all' | 'info' | 'warn' | 'error' }>
}>()

const logLevelFilter = defineModel<'all' | 'info' | 'warn' | 'error'>('logLevelFilter', {
  required: true,
})

const emit = defineEmits<{
  (event: 'clear'): void
}>()

const logLevelSeverity = (level: LogEntry['level']) => {
  if (level === 'info') return 'info'
  if (level === 'warn') return 'warning'
  return 'danger'
}

const levelLabel = (level: LogEntry['level']) => {
  if (level === 'info') return '信息'
  if (level === 'warn') return '警告'
  return '错误'
}
</script>

<style scoped>
.logs-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  width: 100%;
}

.logs-actions {
  display: flex;
  gap: 10px;
  align-items: center;
}

.logs-actions :deep(.p-select) {
  min-width: 0;
  width: 160px;
}

.log-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
  max-height: 500px;
  overflow-y: auto;
}

.log-item {
  padding: 12px;
  border-radius: 6px;
  background: var(--surface-50);
  border-left: 3px solid transparent;
}

.log-item.info {
  border-left-color: var(--blue-500);
}

.log-item.warn {
  border-left-color: var(--yellow-500);
}

.log-item.error {
  border-left-color: var(--red-500);
}

.log-meta {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 6px;
  flex-wrap: wrap;
}

.log-time {
  font-size: 0.75rem;
  color: var(--text-color-secondary);
}

.log-source {
  font-size: 0.75rem;
  color: var(--text-color-secondary);
  font-family: monospace;
  background: var(--surface-100);
  padding: 2px 6px;
  border-radius: 3px;
}

.log-message {
  font-size: 0.875rem;
  color: var(--text-color);
  margin-bottom: 4px;
}

.log-detail {
  background: var(--surface-100);
  padding: 8px;
  border-radius: 4px;
  font-size: 0.75rem;
  overflow-x: auto;
  margin: 4px 0 0 0;
  font-family: 'Courier New', monospace;
  color: var(--text-color-secondary);
}

.empty {
  text-align: center;
  padding: 40px;
  color: var(--text-color-secondary);
  font-size: 0.875rem;
}
</style>
