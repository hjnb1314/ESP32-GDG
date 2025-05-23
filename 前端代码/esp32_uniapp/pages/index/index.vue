<template>
  <view class="container">
    <!-- ngrok 域名设置卡片 -->
    <view class="card" style="margin-bottom:24rpx;">
      <view class="row">
        <input
          v-model="ngrokDomain"
          placeholder="请输入ngrok域名 (如: xxx.ngrok-free.app)"
          class="ngrok-input"
        />
        <button @click="changeNgrok" type="primary" class="mini-btn">切换</button>
      </view>
      <view class="domain-tip">当前连接：{{ wsUrlShow }}</view>
    </view>

    <!-- 环境参数展示卡片 -->
    <view class="card param-card">
      <view class="env-row">
        <text>🌡️ 温度：</text>
        <text class="env-value">{{ env.temperature ?? '--' }} ℃</text>
        <text>💧 湿度：</text>
        <text class="env-value">{{ env.humidity ?? '--' }} %</text>
      </view>
      <view class="env-row">
        <text>🔆 光照：</text>
        <text class="env-value">{{ env.light ?? '--' }}</text>
      </view>
      <view class="env-row">
        <text>📍 纬度：</text>
        <text class="env-value">{{ env.latitude ?? '--' }}</text>
        <text>📍 经度：</text>
        <text class="env-value">{{ env.longitude ?? '--' }}</text>
      </view>
      <view class="env-row">
        <text>🛰️ 卫星数：</text>
        <text class="env-value">{{ env.satellites ?? '--' }}</text>
      </view>
    </view>

    <!-- 连接状态和操作按钮 -->
    <view class="status-card card">
      <view class="ws-status">
        <text>连接状态：</text>
        <text :style="{color: wsStatusColor}">{{ wsStatus }}</text>
      </view>
      <view class="ws-btn-row">
        <button @click="closeWS" type="warn" size="mini" style="margin-right:24rpx;">断开</button>
        <button @click="reconnectWS" type="primary" size="mini">重连</button>
      </view>
    </view>

    <!-- 地图部分固定在最下方，若未获取经纬度则提示 -->
    <view class="map-block card">
      <iframe
        v-if="mapUrl"
        :src="mapUrl"
        width="100%"
        height="380"
        frameborder="0"
        style="border-radius:14rpx;"
      ></iframe>
      <view v-else class="map-empty">暂无定位数据，无法显示地图</view>
    </view>
  </view>
</template>

<script>
export default {
  data() {
    return {
      ngrokDomain: '', // 默认留空，首次进入手动填
      wsTask: null,
      env: {},
      wsStatus: '未连接',
      wsUrlCache: '',
      wsUrlShow: '',
    };
  },
  computed: {
    mapUrl() {
      if (!this.env.latitude || !this.env.longitude) return '';
      // 高德地图 iframe
      return `https://uri.amap.com/marker?position=${this.env.longitude},${this.env.latitude}&name=当前位置`;
    },
    wsStatusColor() {
      // 状态颜色提示
      if (this.wsStatus.indexOf('已连接') !== -1) return '#18b566';
      if (this.wsStatus.indexOf('连接中') !== -1) return '#388aff';
      if (this.wsStatus.indexOf('出错') !== -1 || this.wsStatus.indexOf('断开') !== -1) return '#f56c6c';
      return '#888';
    }
  },
  onLoad() {
    // 优先从本地存储恢复
    const save = uni.getStorageSync('ngrokDomain');
    if (save) this.ngrokDomain = save;
    this.connectWS();
  },
  onUnload() {
    this.closeWS();
  },
  methods: {
    connectWS() {
      // 断开旧ws
      this.closeWS();

      if (!this.ngrokDomain) {
        this.wsStatus = '请先输入后端域名';
        this.wsUrlShow = '';
        return;
      }

      const wsUrl = `wss://${this.ngrokDomain}`;
      this.wsUrlCache = wsUrl;
      this.wsUrlShow = wsUrl;

      try {
        // 用uni.connectSocket，兼容安卓、H5、小程序
        this.wsTask = uni.connectSocket({
          url: wsUrl,
          success: () => { this.wsStatus = '连接中...'; },
          fail: () => { this.wsStatus = '连接出错'; }
        });

        // onOpen/onMessage/onClose/onError
        this.wsTask.onOpen(() => {
          this.wsStatus = '已连接';
        });
        this.wsTask.onMessage((msg) => {
          try {
            this.env = JSON.parse(msg.data);
          } catch {
            this.env = {};
          }
        });
        this.wsTask.onError(() => {
          this.wsStatus = '连接出错';
        });
        this.wsTask.onClose(() => {
          this.wsStatus = '连接已断开';
        });

      } catch (e) {
        this.wsStatus = 'WebSocket连接异常';
      }
    },
    changeNgrok() {
      if (!/^[\w\.\-]+\.ngrok\-free\.app$/.test(this.ngrokDomain)) {
        uni.showToast({ title: 'ngrok域名格式无效', icon: 'none' });
        return;
      }
      uni.setStorageSync('ngrokDomain', this.ngrokDomain);
      this.connectWS();
      uni.showToast({ title: 'WebSocket已切换', icon: 'none' });
    },
    closeWS() {
      if (this.wsTask) {
        try { this.wsTask.close(); } catch (e) {}
        this.wsTask = null;
      }
      this.wsStatus = '已断开';
    },
    reconnectWS() {
      this.connectWS();
    }
  }
}
</script>

<style>
.container {
  padding: 24rpx 14rpx 12rpx 14rpx;
  background: #f6f8fa;
  min-height: 100vh;
}
.card {
  background: #fff;
  border-radius: 16rpx;
  box-shadow: 0 4rpx 14rpx #ebedf3;
  padding: 22rpx 20rpx;
  margin-bottom: 28rpx;
}
.row {
  display: flex;
  align-items: center;
  flex-wrap: wrap;
}
.ngrok-input {
  flex: 1;
  min-width: 220rpx;
  max-width: 450rpx;
  border: 1rpx solid #d8d8d8;
  border-radius: 8rpx;
  padding: 12rpx 16rpx;
  font-size: 30rpx;
  margin-right: 12rpx;
  background: #f8fafd;
}
.mini-btn {
  font-size: 28rpx;
  padding: 0 22rpx;
  height: 60rpx;
  border-radius: 8rpx;
}
.domain-tip {
  margin-top: 8rpx;
  color: #8d99a7;
  font-size: 22rpx;
  word-break: break-all;
}
.param-card .env-row {
  display: flex;
  align-items: center;
  margin-bottom: 8rpx;
  font-size: 30rpx;
}
.env-value {
  color: #18b566;
  font-weight: bold;
  margin-right: 20rpx;
}
.status-card {
  padding: 20rpx 18rpx;
  display: flex;
  justify-content: space-between;
  align-items: center;
}
.ws-status {
  font-size: 30rpx;
  font-weight: 500;
}
.ws-btn-row {
  display: flex;
  align-items: center;
}
.map-block {
  margin-top: 34rpx;
  padding: 0;
  background: #fff;
  overflow: hidden;
  box-shadow: 0 4rpx 14rpx #e8ecf2;
  border-radius: 18rpx;
}
.map-empty {
  text-align: center;
  color: #aaa;
  font-size: 26rpx;
  padding: 34rpx 0 36rpx 0;
}
</style>
