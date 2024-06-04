//Thiết lập thời gian--------------------------------------------//
function updateTime() {
    var currentDate = new Date();
    var hours = currentDate.getHours();
    var minutes = currentDate.getMinutes();
    var seconds = currentDate.getSeconds();
    var day = currentDate.getDate();
    var month = currentDate.getMonth() + 1;
    var year = currentDate.getFullYear();
    var timeString = formatTime(hours) + ":" + formatTime(minutes) + ":" + formatTime(seconds);
    var dateString = formatTime(day) + "/" + formatTime(month) + "/" + year;
    var timeElements = document.getElementsByClassName("time");
    var dateElements = document.getElementsByClassName("date");
    for (var i = 0; i < timeElements.length; i++) {
      timeElements[i].innerHTML = timeString;
    }
    for (var j = 0; j < dateElements.length; j++) {
      dateElements[j].innerHTML = dateString;
    }
  }
function formatTime(time) {
    return time < 10 ? "0" + time : time;
}
function function_Up() {
  var currentAngle = parseInt(document.getElementById("donang").innerText);
  var newAngle = currentAngle + 1; // Tăng mức nâng lên 1 (có thể điều chỉnh theo nhu cầu)
  if (newAngle > 3) {
      newAngle = 3; // Giới hạn mức nâng
  }
  document.getElementById("donang").innerText = newAngle;
  // Gửi dữ liệu góc mới lên Firebase
  firebase.database().ref('thietbi1').update({
      donang: newAngle
  });
}

function function_Down() {
  var currentAngle = parseInt(document.getElementById("donang").innerText);
  var newAngle = currentAngle - 1; // Giảm mức nâng xuống 1 mức (có thể điều chỉnh theo nhu cầu)
  if (newAngle < 0) {
      newAngle = 0; // Giới hạn mức nâng tối thiểu
  }
  document.getElementById("donang").innerText = newAngle;
  // Gửi dữ liệu mức nâng mới lên Firebase
  firebase.database().ref('thietbi1').update({
      donang: newAngle
  });
}
