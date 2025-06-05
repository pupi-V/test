document.addEventListener('DOMContentLoaded', function() {
    // Автообновление страницы каждые 10 секунд
    setTimeout(function() {
        location.reload();
    }, 10000);

    // Для каждой формы отменяем стандартную отправку и отправляем запрос через fetch
    document.querySelectorAll('form').forEach(function(form) {
        form.addEventListener('submit', function(e) {
            e.preventDefault(); // Остановить переход/перезагрузку

            const formData = new FormData(form);
            fetch(form.action, {
                method: 'POST',
                body: formData
            })
            .then(response => {
                // Можно обработать ответ, если нужно
                console.log('Запрос отправлен на', form.action);
            })
            .catch(error => {
                console.error('Ошибка отправки:', error);
            });
        });
    });
});


