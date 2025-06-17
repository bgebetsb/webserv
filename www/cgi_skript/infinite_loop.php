<?php
// infinite_loop.php
set_time_limit(0); // prevent timeout from PHP itself
while (true) {
    // Simulate CPU work
    usleep(100000); // 0.1 seconds
    echo "Still running...\n";
    flush();
}
?>
