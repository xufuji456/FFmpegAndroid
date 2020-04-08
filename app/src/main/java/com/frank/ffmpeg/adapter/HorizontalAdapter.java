package com.frank.ffmpeg.adapter;

import android.graphics.Color;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import com.frank.ffmpeg.R;
import com.frank.ffmpeg.listener.OnItemClickListener;

import java.util.List;

/**
 * the horizontal adapter of RecyclerView
 * Created by frank on 2018/6/6.
 */

public class HorizontalAdapter extends RecyclerView.Adapter {

    private List<String> itemList;
    private OnItemClickListener onItemClickListener;
    private int lastClickPosition;

    public HorizontalAdapter(List<String> itemList) {
        this.itemList = itemList;
    }

    public void setOnItemClickListener(OnItemClickListener onItemClickListener) {
        this.onItemClickListener = onItemClickListener;
    }

    @NonNull
    @Override
    public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        return new OkViewHolder(LayoutInflater.from(parent.getContext()).
                inflate(R.layout.item_select, parent, false));
    }

    @Override
    public void onBindViewHolder(RecyclerView.ViewHolder holder, int position) {
        final OkViewHolder okViewHolder = (OkViewHolder) holder;
        okViewHolder.btn_select.setText(itemList.get(position));
        okViewHolder.btn_select.setTextColor(Color.DKGRAY);
        if (onItemClickListener != null) {
            okViewHolder.btn_select.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    notifyItemChanged(lastClickPosition);
                    Log.i("onBindViewHolder", "lastClickPosition=" + lastClickPosition);
                    //设置当前选中颜色
                    okViewHolder.btn_select.setTextColor(Color.BLUE);
                    onItemClickListener.onItemClick(okViewHolder.getAdapterPosition());
                    lastClickPosition = okViewHolder.getAdapterPosition();
                }
            });
        }
    }

    @Override
    public int getItemCount() {
        return itemList != null ? itemList.size() : 0;
    }

    private class OkViewHolder extends RecyclerView.ViewHolder {
        Button btn_select;

        OkViewHolder(View itemView) {
            super(itemView);
            btn_select = itemView.findViewById(R.id.btn_select);
        }
    }

}
