<?php
$temp_file = "card.log"; // 卡片临时存储
$log_file = "open.log";// 刷卡日志
$card_file = "card.json";// 卡片数据
$web_file = "web.html";// 网页模板

$card = json_decode(file_get_contents($card_file),true);
if(isset($_GET["uid"]) && $_GET["uid"]){
    $uid = $_GET["uid"];
    $status = "fail";
    $last = file_get_contents($temp_file);
    foreach ($card as $v){
        if($v["uid"] != $uid)
            continue;
        $status = "pass";
        break;
    }
    if($last != $uid || $status == "pass"){
        file_put_contents($temp_file,$uid);
        file_put_contents($log_file,"\r\n".$status."-".$uid."-".time(),FILE_APPEND);
    }
    exit($status);
}
if(!isset($_GET["do"]))
    $_GET["do"] = "index";
    
switch ($_GET["do"]) {
    case 'read':
        exit(file_get_contents($temp_file));
        break;
    case 'add':
        $card[] = [
            "name" => $_GET["name"],
            "uid"  => $_GET["id"]
            ];
        file_put_contents($card_file,json_encode($card,JSON_UNESCAPED_UNICODE));
        exit("success");
        break;
    default:break;
}
$html = file_get_contents($web_file);
$log = fopen($log_file, 'r') or die('Unable to open log file!');
$log_html = "";$card_html="";$eof="";$pos=-2;
file_put_contents($temp_file,"");
for ($i = 0;$i < 10;$i++) {
    while($eof != "\n"){
        if(!fseek($log,$pos,SEEK_END)){
            $eof=fgetc($log);
            $pos--;
        }else break;
    }
    $eof="";
    $temp = fgets($log);
    if(empty($temp))break;
    $temp = explode("-",$temp,3);
    $log_html.="<li class=\"mdui-list-item\"><div class=\"mdui-list-item-content\"><div class=\"mdui-list-item-title\">";
    switch($temp[0]){
        case 'pass':$log_html.="成功开门";break;
        case 'fail':$log_html.="无法识别的卡片";break;
        case 'add':$log_html.="新增卡片";break;
    }
    foreach ($card as $v){
        if($v["uid"] != $temp[1])
            continue;
        $log_html.=" - ".$v["name"];
        break;
    }
    $log_html.="</div><div class=\"mdui-list-item-text mdui-list-item-two-line\">";
    $log_html.="卡片UID：".$temp[1]."<br>";
    $log_html.="操作时间:".date("Y年m月d号 H:i:s",$temp[2]);
    $log_html.="</div></div></li>";								
}
fclose($log);
foreach ($card as $v){
    $card_html.="<li class=\"mdui-list-item mdui-ripple\"><div class=\"mdui-list-item-content\"><div class=\"mdui-list-item-title\">";
    $card_html.=$v["name"]."</div><div class=\"mdui-list-item-text\">";
    $card_html.="卡片UID：".$v["uid"]."</div></div><i class=\"mdui-list-item-icon mdui-icon material-icons\">delete</i></li>";
}
$html = str_replace(array("{{log}}","{{card}}","{{card_count}}"),array($log_html,$card_html,count($card)),$html);
echo $html;
