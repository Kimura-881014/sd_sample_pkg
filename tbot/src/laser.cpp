#include <iostream>

#include <ros/ros.h>
#include <sensor_msgs/LaserScan.h> // scan message  
#include <geometry_msgs/Twist.h>   // velocity message

// クラスLaserSampleNodeの定義
class LaserSampleNode
{
private:
// ノードハンドラの定義   
  ros::NodeHandle nh;

  ros::Subscriber sub_laser;
  ros::Publisher pub_vel;

  // laser scanの測定値を保存する変数の定義
  sensor_msgs::LaserScan latest_scan;

  // 速度指令値を保存する変数の定義
  geometry_msgs::Twist cmd_msg;

  // 2Hzで制御ループを回す
  const int loop_rate = 2;

  // メッセージの初回受信を確認する変数
  bool is_recieved_scan = false;

public:
  LaserSampleNode()
  {
    sub_laser = nh.subscribe("/light_sensor/front/scan", 10, &LaserSampleNode::laser_callback, this);
    pub_vel = nh.advertise<geometry_msgs::Twist>("/cmd_vel", 10);
  }

  // 速度指令値を保持する変数に値を代入する関数
  geometry_msgs::Twist create_vel_msg(const double vel, const double omega)
  {
    // 速度指令値を格納する変数の定義
    // http://docs.ros.org/en/melodic/api/geometry_msgs/html/msg/Twist.html
    geometry_msgs::Twist cmd_msg;
    cmd_msg.linear.x = vel;
    cmd_msg.angular.z = omega;

    return cmd_msg;
  }

  // laserトピックを受けたときに実行される関数
  // 最新のスキャン情報を保存しておく
  void laser_callback(const sensor_msgs::LaserScanConstPtr &laser_msg)
  {
    latest_scan = *laser_msg;
    is_recieved_scan = true;
  }

  void mainloop()
  {
    ros::Rate r(loop_rate);
    int cnt = 0;

    while (ros::ok())
    {
      ros::spinOnce();

      // 回転判定してから一定回数は回転司令を与え続ける
      cnt++;
      if (cnt < 4)
      {
        pub_vel.publish(cmd_msg);
        continue;
      }

      // 値を取得しないままlatest_scan変数にアクセスするとエラーを起こすので判定
      if(!is_recieved_scan) continue;

      // 複数あるセンサの値のうち、中央にある値を取得する
      // latest_scan.rangesはvectorなので、中央の添字で真ん中の距離を取得可能
      // http://docs.ros.org/en/api/sensor_msgs/html/msg/LaserScan.html
      const double center_value = latest_scan.ranges[latest_scan.ranges.size() / 2];

      // センサの中央の距離が近かった場合
      if (center_value < 0.5)
      {
        // 0.0 m/s、0.5 rad/s の速度指令値を作り、送信
        // その場で回転する
        cmd_msg = create_vel_msg(0.0, 0.5);
        pub_vel.publish(cmd_msg);

        cnt=0;

        ROS_INFO("center laser value %5f : rotate", center_value);
      }
      else
      {
        // 0.3 m/s、0.0 rad/s の速度指令値を作り、送信
        // まっすぐ進む
        cmd_msg = create_vel_msg(0.3, 0.0);
        pub_vel.publish(cmd_msg);

        ROS_INFO("center laser value %8f : move on", center_value);
      }

      // 指定したループ周期になるように調整
      r.sleep();
    }
  }
};

int main(int argc, char **argv)
{

  ros::init(argc, argv, "laser_sample_node");

  // クラスLaserSampleNodeの実体となる変数nodeの定義
  LaserSampleNode node = LaserSampleNode();

  // メインループを回す
  node.mainloop();

  return 0;
}