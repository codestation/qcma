package com.github.codestation.qcma;

import android.content.Context;
import android.content.DialogInterface;
import android.widget.Toast;
import android.app.AlertDialog;

public class QcmaNotification extends org.qtproject.qt5.android.bindings.QtActivity
{
    private static QcmaNotification m_instance;

    public QcmaNotification()
    {
        m_instance = this;
    }

    public static void showToast(String message)
    {
        Context context = m_instance.getApplicationContext();
        Toast toast = Toast.makeText(context, message, Toast.LENGTH_SHORT);
        toast.show();
    }

    public static void showDialog(String message)
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(m_instance);
        builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            }
        });
        builder.setMessage(message).setTitle("QCMA message");
        AlertDialog dialog = builder.create();
    }
}
