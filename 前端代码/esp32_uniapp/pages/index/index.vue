<template>
  <view class="container">
    <view class="block" style="margin-bottom:20rpx;">
      <input
        v-model="ngrokDomain"
        placeholder="请输入ngrok域名(如 ca77-219-128-230-37.ngrok-free.app)"
        class="ngrok-input"
      />
      <button @click="changeNgrok">切换WebSocket地址</button>
    </view>

    <view class="block"><text>🌡️ 温度：{{ env.temperature }} ℃</text></view>
    <view class="block"><text>💧 湿度：{{ env.humidity }} %</text></view>
    <view class="block"><text>🔆 光照：{{ env.light }}</text></view>
    <view class="block"><text>📍 纬度：{{ env.latitude }}</text></view>
    <view class="block"><text>📍 经度：{{ env.longitude }}</text></view>
    <view class="block"><text>🛰️ 卫星数：{{ env.satellites }}</text></view>

    <view class="status" style="margin-top:30rpx;">
      <text>连接状态：{{ wsStatus }}</text>
    </view>
    <button @click="closeWS" type="warn" style="margin-top:20rpx;">断开WebSocket</button>
    <button @click="reconnectWS" style="margin-top:10rpx;">重新连接</button>
  </view>
</template>

<script>
export default {
  data() {
    return {
      ngrokDomain: 'ca77-219-128-230-37.ngrok-free.app', // 默认ngrok域名
      ws: null,
      env: {},
      wsStatus: '未连接',
      wsUrlCache: ''
    };
  },
  onLoad() {
    this.connectWS();
  },
  onUnload() {
    this.closeWS();
  },
  methods: {
    // 动态拼接WebSocket地址并连接
    connectWS() {
      // 关闭旧连接
      if (this.ws) {
        this.ws.close();
        this.ws = null;
      }
      if (!this.ngrokDomain) {
        this.wsStatus = '请先输入后端域名';
        return;
      }
      const wsUrl = `wss://${this.ngrokDomain}`;
      this.wsUrlCache = wsUrl;
      try {
        this.ws = new WebSocket(wsUrl);
        this.wsStatus = '连接中...';
        this.ws.onopen = () => {
          this.wsStatus = '已连接';
          console.log('WebSocket已连接:', wsUrl);
        };
        this.ws.onmessage = (e) => {
          try {
            this.env = JSON.parse(e.data);
          } catch {
            this.env = {};
          }
        };
        this.ws.onerror = (err) => {
          this.wsStatus = '连接出错';
          console.log('WebSocket错误:', err);
        };
        this.ws.onclose = () => {
          this.wsStatus = '连接已关闭';
          console.log('WebSocket已关闭');
        };
      } catch (e) {
        this.wsStatus = 'WebSocket连接异常';
        console.error('WebSocket异常:', e);
      }
    },
    changeNgrok() {
      this.connectWS();
      uni.showToast({ title: 'WebSocket已切换', icon: 'none' });
    },
    closeWS() {
      if (this.ws) {
        this.ws.close();
        this.ws = null;
        this.wsStatus = '已断开';
      }
    },
    reconnectWS() {
      this.connectWS();
    }
  }
};
</script>

<style>
.container {
  padding: 30rpx;
}
.block {
  margin-bottom: 24rpx;
  font-size: 38rpx;
}
.ngrok-input {
  width: 420rpx;
  padding: 12rpx;
  font-size: 28rpx;
  border: 1rpx solid #ddd;
  border-radius: 8rpx;
  margin-right: 16rpx;
  margin-bottom: 16rpx;
}
.status {
  font-size: 28rpx;
  color: #888;
}
</style>
