<template>
  <Card class="status-card">
    <template #title>
      <span class="card-title">核心状态</span>
    </template>
    <template #content>
      <div class="status-grid">
        <div>
          <div class="status-label">核心连接</div>
          <div class="status-value">
            <Tag
              :severity="isOnline ? 'success' : 'secondary'"
              :value="isOnline ? '已连接' : '等待核心响应...'"
            />
          </div>
        </div>
        <div>
          <div class="status-label">当前任务</div>
          <div class="status-value">{{ statusSummary.name || '-' }}</div>
        </div>
        <div>
          <div class="status-label">状态</div>
          <div class="status-value">{{ statusSummary.status || '-' }}</div>
        </div>
        <div>
          <div class="status-label">进度</div>
          <div class="status-value">{{ statusSummary.progress ?? '-' }}</div>
        </div>
        <div>
          <div class="status-label">最近事件</div>
          <div class="status-value">{{ lastEventText }}</div>
        </div>
      </div>
    </template>
  </Card>
</template>

<script setup lang="ts">
import Card from 'primevue/card'
import Tag from 'primevue/tag'
import type { StatusSummary } from './types'

defineProps<{
  isOnline: boolean
  statusSummary: StatusSummary
  lastEventText: string
}>()
</script>

<style scoped>
.status-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16px;
}

.status-label {
  font-size: 0.875rem;
  color: var(--text-color-secondary);
  margin-bottom: 4px;
}

.status-value {
  font-size: 1rem;
  color: var(--text-color);
  font-weight: 500;
}
</style>
